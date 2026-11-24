"""Reproduce Windows updater's Git failure in a real single-branch clone.
Executes the same Git commands read from INSTALAR.bat; does not emulate cmd.exe.
"""
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
TARGET = "test/vtts-pt-br-hw04"


class BranchSwitchTest(unittest.TestCase):
    def git(self, cwd, *args, check=True):
        return subprocess.run(["git", *args], cwd=cwd, text=True, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, check=check)

    def installer_command(self, prefix):
        lines = (ROOT / "INSTALAR.bat").read_text().splitlines()
        line = next(line.strip() for line in lines if line.strip().startswith(prefix))
        return shlex.split(line.replace("!TARGET_BRANCH!", TARGET))[1:]

    def fixture(self, directory):
        root = Path(directory)
        seed, remote, clone = root / "seed", root / "remote.git", root / "clone"
        seed.mkdir()
        self.git(root, "init", "--bare", str(remote))
        self.git(seed, "init", "-b", "develop")
        self.git(seed, "config", "user.name", "Installer Test")
        self.git(seed, "config", "user.email", "installer@example.invalid")
        (seed / "version.txt").write_text("develop\n")
        self.git(seed, "add", ".")
        self.git(seed, "commit", "-m", "develop")
        self.git(seed, "switch", "-c", TARGET)
        (seed / "version.txt").write_text("isolated lab\n")
        self.git(seed, "commit", "-am", "isolated lab")
        self.git(seed, "push", str(remote), "--all")
        self.git(root, "clone", "--single-branch", "--branch", "develop", remote.as_uri(), str(clone))
        return clone

    def test_single_branch_failure_then_fixed_switch_and_pull(self):
        with tempfile.TemporaryDirectory() as tmp:
            clone = self.fixture(tmp)
            self.git(clone, *self.installer_command("git fetch --no-tags --prune origin"))
            failed = self.git(clone, "switch", "--track", "-c", TARGET, "origin/" + TARGET, check=False)
            self.assertNotEqual(failed.returncode, 0)
            self.assertIn("is not a branch", failed.stderr)
            self.git(clone, *self.installer_command("git config --replace-all remote.origin.fetch"))
            self.git(clone, *self.installer_command("git fetch --no-tags --prune origin"))
            self.git(clone, *self.installer_command("git switch --track -c"))
            self.git(clone, *self.installer_command("git branch --set-upstream-to="))
            self.git(clone, *self.installer_command("git merge --ff-only"))
            self.assertEqual(self.git(clone, "rev-parse", "--abbrev-ref", "@{upstream}").stdout.strip(), "origin/" + TARGET)
            self.assertEqual((clone / "version.txt").read_text(), "isolated lab\n")
            self.git(clone, "pull", "--ff-only")
            self.assertEqual(self.git(clone, "status", "--porcelain").stdout, "")

    def test_existing_local_branch_and_duplicate_refspecs(self):
        with tempfile.TemporaryDirectory() as tmp:
            clone = self.fixture(tmp)
            self.git(clone, "config", "--add", "remote.origin.fetch", "+refs/heads/main:refs/remotes/origin/main")
            self.git(clone, *self.installer_command("git config --replace-all remote.origin.fetch"))
            self.assertEqual(self.git(clone, "config", "--get-all", "remote.origin.fetch").stdout.strip(), "+refs/heads/*:refs/remotes/origin/*")
            self.git(clone, *self.installer_command("git fetch --no-tags --prune origin"))
            self.git(clone, "branch", "--no-track", TARGET, "origin/" + TARGET)
            self.git(clone, "switch", TARGET)
            self.git(clone, *self.installer_command("git branch --set-upstream-to="))
            self.git(clone, *self.installer_command("git merge --ff-only"))
            self.git(clone, "pull", "--ff-only")


if __name__ == "__main__":
    unittest.main()
