from __future__ import annotations

import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 4:
        print("Uso: ensure_sdkconfig.py <arquivo> <opcao> <valor>")
        return 2

    path = Path(sys.argv[1])
    option = sys.argv[2].strip()
    value = sys.argv[3].strip()

    if not option.startswith("CONFIG_"):
        print(f"[ERRO] Opcao invalida: {option}")
        return 2

    if not path.exists():
        print(f"[INFO] {path} ainda nao existe; sdkconfig.defaults sera usado na primeira geracao.")
        return 0

    raw = path.read_text(encoding="utf-8", errors="ignore")
    lines = raw.splitlines()
    enabled_prefix = option + "="
    disabled_line = f"# {option} is not set"

    out: list[str] = []
    replaced = False
    for line in lines:
        stripped = line.strip()
        if stripped.startswith(enabled_prefix) or stripped == disabled_line:
            if not replaced:
                out.append(f"{option}={value}")
                replaced = True
            continue
        out.append(line)

    if not replaced:
        if out and out[-1] != "":
            out.append("")
        out.append(f"{option}={value}")

    path.write_text("\n".join(out).rstrip() + "\n", encoding="utf-8")
    print(f"[OK] {option}={value} em {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
