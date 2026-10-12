"""Host mocks: migration/status only; not ESP-IDF or hardware validation."""
from pathlib import Path
import shutil, subprocess, tempfile
root=Path(__file__).resolve().parents[2]
here=Path(__file__).resolve().parent
cc=shutil.which("cc") or shutil.which("gcc")
if not cc: raise SystemExit("GCC/Clang required")
with tempfile.TemporaryDirectory(prefix="jrbot_wifi_") as tmp:
    binary=Path(tmp)/"wifi_integration"
    subprocess.run([cc,"-std=c11","-Wall","-Wextra","-Werror","-fsanitize=undefined",
        "-I",str(here),"-I",str(root/"firmware/core"),
        str(here/"test_wifi_integration.c"),"-o",str(binary)],check=True)
    for case in ("fresh","legacy","experiment","deleted","pending_config","pending_clear","serial_save","serial_clear"):
        subprocess.run([str(binary),case],check=True)
