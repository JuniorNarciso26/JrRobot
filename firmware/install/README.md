# JrBot — Firmware Installation Notes

**Historical installation record:** these instructions refer to an older V2 diagnostic environment and must not be treated as the current installation procedure.

The original workflow used a revised V2 branch and `DIAG_V2.bat` at the repository root. In that environment, `build` compiled the firmware and `flash` compiled and flashed it through the serial port selected on the computer. The panel was opened through `PAINEL.bat` using its selected serial port, without requiring a separate monitor.

The original guide also required reviewing the historical `APLICAR_BRANCH_V2.md` instructions and the [HW04 Wiring Diagram](../../docs/02-assembly/WIRING.md) before enabling additional modules. Older audio configurations were not approved in the HW03 revision.

For current development and installation, use the [Development Guide](../../docs/04-firmware-development/DEVELOPMENT.md) and verify the actual build installed on the device.
