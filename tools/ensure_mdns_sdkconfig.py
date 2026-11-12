"""Apply only the mDNS settings for the staged develop_wifi build."""
from __future__ import annotations

from pathlib import Path
import re
import sys

OPTIONS = {
    "CONFIG_MDNS_MEMORY_ALLOC_SPIRAM": "y",
    "CONFIG_MDNS_MEMORY_ALLOC_INTERNAL": "n",
    "CONFIG_MDNS_TASK_CREATE_FROM_INTERNAL": "y",
    "CONFIG_MDNS_TASK_CREATE_FROM_SPIRAM": "n",
    "CONFIG_MDNS_MEMORY_CUSTOM_IMPL": "n",
    "CONFIG_MDNS_TASK_STACK_SIZE": "4096",
    "CONFIG_MDNS_MAX_INTERFACES": "1",
    "CONFIG_MDNS_MAX_SERVICES": "1",
    "CONFIG_MDNS_PREDEF_NETIF_STA": "y",
    "CONFIG_MDNS_PREDEF_NETIF_AP": "n",
    "CONFIG_MDNS_PREDEF_NETIF_ETH": "n",
    "CONFIG_MDNS_NETWORKING_SOCKET": "n",
    "CONFIG_MDNS_ENABLE_BROWSE": "n",
    "CONFIG_MDNS_ENABLE_CONSOLE_CLI": "n",
}


def apply(path: Path) -> None:
    if not path.exists():
        print("[INFO] mDNS: sdkconfig.defaults sera usado na primeira geracao.")
        return
    lines = path.read_text(encoding="utf-8").splitlines()
    pattern = re.compile(r"^(?:# )?(CONFIG_MDNS_[A-Z0-9_]+)(?:=| is not set$)")
    result = []
    for line in lines:
        match = pattern.match(line.strip())
        if not match or match[1] not in OPTIONS:
            result.append(line)
    while result and not result[-1].strip():
        result.pop()
    result.append("")
    for option, value in OPTIONS.items():
        result.append(f"# {option} is not set" if value == "n" else f"{option}={value}")
    path.write_text("\n".join(result).rstrip() + "\n", encoding="utf-8")
    print("[OK] mDNS: buffers PSRAM, stack interna, somente STA/HTTPS; demais ajustes preservados.")


def main() -> int:
    if len(sys.argv) != 2:
        print("Uso: ensure_mdns_sdkconfig.py <sdkconfig>")
        return 2
    apply(Path(sys.argv[1]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
