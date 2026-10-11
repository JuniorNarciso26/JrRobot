# Testing and Validation

JrBot is a physical robotics project. Successfully compiling firmware does **not** prove physical behavior.

## Evidence levels

### 1. Planned

Only design or documentation exists.

### 2. Implemented

Code exists, without verified build or hardware behavior.

### 3. Build verified

The project compiles and packages successfully for the intended target.

### 4. Bench or simulated

The logical path was exercised in simulation, through diagnostics, or with a mock.

### 5. Physically validated

The function was directly observed on real hardware.

## Examples

### Voice recognition

Simulating a wake word through `brain_test` demonstrates integration with the brain/face path, **not** acoustic recognition.

A log containing:

```text
name detected ... probability=...
```

during a real spoken test may support acoustic validation of that specific test.

### Audio

```text
PLAY_RECORDING_END result=ESP_OK
```

confirms completion of the software/I2S path. To claim the sound was audible, record human confirmation during the physical test.

### Camera

A valid JPEG confirms digital capture but does not prove good focus, exposure, or visual quality.

## Voice candidate checklist

1. Confirm the board version.
2. Confirm the loaded model.
3. Confirm the microphone is active.
4. Perform multiple real spoken trials.
5. Record confidence/probability scores.
6. Speak unrelated phrases.
7. Test noise, television, and silence when relevant.
8. Check audio responses.
9. Confirm listening resumes afterward.
10. Look for reboots and assertions.
11. Save the full log.

## Playground calibration

A calibration session should distinguish actual positive spoken attempts, negative/background speech or noise, received candidates, confidence scores, and detections accepted by the selected configuration.

Choose a threshold that balances false positives and false negatives based on actual hardware measurements.

## Project criteria versus model guarantees

Internal targets such as "8/10 detections and 0/20 false positives" may be used as project acceptance criteria, but they must be described as **JrBot targets**, not guarantees from ESP-SR.

## Minimum test report

```text
Firmware:
Hardware:
Branch/commit:
Date:
Environment:
Procedure:
Expected result:
Observed result:
Logs:
Physical validation:
Issues found:
```
