# JrBot Playground — Future Design

**Status: PLANNED.** Playground is a proposed calibration and behavior-authoring interface, not an implemented feature of the stable firmware. It would **not train a neural network on the ESP32**.

## Intended user flow

1. **Enter a phrase** (for example, `JR BOT`). Load it temporarily for testing without triggering normal robot actions.
2. **Start a test session.** The microphone would listen continuously; an interval such as 2 seconds would limit **sample reporting**, not switch the microphone on and off.
3. **Review measurements.** Show recognized phrases, confidence scores, minimum, maximum and average. Include positive and negative/background trials before choosing a threshold.
4. **Save explicitly.** Store the chosen phrase and confidence only after user confirmation.
5. **Build behavior.** Associate approved actions with the calibrated trigger, ideally through a visual editor. The [Flow Engine](FLOWS.md) describes the proposed behavior model.

## Illustrative samples

| Trial | Recognized phrase | Confidence |
| ---: | --- | ---: |
| 1 | JR BOT | 0.751 |
| 2 | JR BOT | 0.624 |
| 3 | JR BOT | 0.583 |
| 4 | JR BOT | 0.702 |

Illustrative setting: `minimum_confidence=0.58`. These sample numbers are **examples, not measurements from a documented hardware test**.

## Planned session model

```text
IDLE → STARTING → CALIBRATING → STOPPING → REVIEW → SAVED / DISCARDED
```

Possible in-memory structure:

```json
{"session":"pg_001","phrase":"JR BOT","interval_ms":2000,"samples":[]}
```

## Safety and persistence

During calibration, report measurements only: **do not trigger speech output, expressions, actuators or ordinary Flows** based on repeated test phrases. Begin in RAM; only explicit Save may persist a configuration, and invalid or incomplete writes must not prevent startup.

Future features may include multiple phrases per intent, profiles, calibration history, WAV assets, import/export, a visual Flow editor, and `capabilities()` integration. Those features remain **unimplemented**.

For acceptance and physical test evidence, see [Testing and Validation](TESTING.md).
