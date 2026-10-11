# JrBot Hardware Assembly

Build the **electronic hardware** of JrBot using the documented **JRBOT-HW-04** configuration. This directory covers components, wiring, installation checks, and the validated hardware revision. The separately documented [3D-printed body](../03-3d-printing/README.md) belongs to the mechanical build.

## Start here

1. **[Bill of Materials](BILL_OF_MATERIALS.md)** — identify the electronic components and accessories.
2. **[Wiring Reference](WIRING.md)** — connect the OLED, camera, microphone, amplifier and speaker using the HW04 pin map.
3. **[Assembly Guide](ASSEMBLY_GUIDE.md)** — follow a practical, step-by-step sequence and check the robot before powering it.
4. **[HW04 Hardware Reference](HARDWARE_HW04.md)** — consult the confirmed revision, constraints and validation notes.

## Source of truth

The machine-readable map is [`hardware/pinmap.json`](../../hardware/pinmap.json); firmware constants are generated in [`jr_pinmap_fixed.h`](../../firmware/core/jr_pinmap_fixed.h). Hardware changes must keep these sources and the [wiring reference](WIRING.md) consistent.

The [original HW04 assembly confirmation](../../hardware/CONFIRMAR_MONTAGEM.txt) remains available for traceability. The current firmware includes microphone and OLED support; any historical mention that these drivers were absent or disabled describes an **older diagnostic stage**.

**Status:** HW04 electronic connections are documented and core functions were physically exercised in the V1.7.04 baseline. This documentation is not a claim that a new consolidated V1.8S hardware test has been performed.
