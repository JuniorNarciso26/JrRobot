# JrBot Electronic Assembly Guide — HW04

Follow these steps with the board **disconnected from power**. Use [Wiring Reference](WIRING.md) for the exact module pins; those connections are listed only there.

## Assembly

1. **Prepare the components.** Check the [Bill of Materials](BILL_OF_MATERIALS.md), controller revision and printed pin labels on each module.
2. **Install the camera.** Seat the OV5640 cable in the controller's dedicated camera connector, checking the connector orientation for your board.
3. **Connect the OLED.** Wire the SSD1306 I2C display according to [Wiring Reference](WIRING.md).
4. **Connect the microphone.** Wire the MS3625 I2S module as shown in the reference.
5. **Connect the amplifier and speaker.** Wire the MAX98357A and connect the speaker **between OUT+ and OUT-**, never to ground. The amplifier and microphone share I2S clock signals by design.

## Before powering on

- Verify supply voltage, ground, connector orientation and all wires against [Wiring Reference](WIRING.md).
- Check for accidental shorts with the board unpowered.
- Do not attach a battery, motor or servo using an unverified circuit.

## Install and test

Use the [Firmware Development Guide](../04-firmware-development/DEVELOPMENT.md) and the project installer. **Select the USB/Serial port identified by your computer**; no fixed COM number is part of JrBot's hardware design.

After flashing, verify the firmware version and physically check:

- **OLED:** displays an expression.
- **Camera:** provides a visible image.
- **Microphone:** records audible input.
- **Speaker:** plays audible output.
- **Local Live:** displays video and passes audio in both directions when supported by the installed firmware.

Record the results as described in [Testing and Validation](../04-firmware-development/TESTING.md). A successful build or status response alone does not confirm physical behavior.

For the mechanical build, continue with [3D Printing and Body](../03-3d-printing/README.md). This guide uses the previously documented HW04 setup; the consolidated V1.8S build requires its own release checks.
