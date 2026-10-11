# JrBot Playground

**Status: PLANNED / design approved for development.**

The Playground is intended for calibrating and constructing JrBot behavior. It **does not train a neural network on the ESP32**. Instead, it configures phrases, measures an available speech recognizer, and builds runtime rules/Flows.

## Step 1 — register a word or phrase

The user enters a phrase, for example:

```text
JR BOT
```

The phrase is loaded temporarily into the recognition engine for a calibration session. No robot action should execute at this stage.

## Step 2 — start a continuous test session

The user selects a minimum interval between recorded results, for example:

```text
2 seconds
```

Then clicks **Start test**.

### Important rule

The microphone continues listening continuously. The interval does **not** mean "switch on the microphone every two seconds"; it sets the minimum cadence at which new observations are accepted or recorded.

The user can repeat tests while varying speaking speed, distance, volume, intonation, position relative to the microphone, and environmental noise.

The session ends only when the user clicks **Stop test**.

## Step 3 — analyze and save confidence

The panel displays each sample:

| # | Recognized | Confidence |
| ---: | --- | ---: |
| 1 | JR BOT | 0.751 |
| 2 | JR BOT | 0.624 |
| 3 | JR BOT | 0.583 |
| 4 | JR BOT | 0.702 |

It can also calculate the count, minimum, maximum, and average. The system may propose a threshold in the future, but the user selects the minimum confidence required by the rule.

```text
Minimum confidence: 0.58
```

**Save confidence** persists the validated phrase/rule configuration.

## Step 4 — build the behavior

After calibration, the user associates actions:

```text
voice("JR BOT", confidence=0.58)
-> face("happy")
-> say("hi")
-> listen()
```

The Playground should first offer a visual interface, with a future advanced mode for editing the full Flow representation.

## Calibration mode

While testing a phrase:

- Do not execute `say`.
- Do not change expressions in response to recognition.
- Do not move actuators.
- Do not trigger normal Flows based on the phrase being calibrated.
- Publish only measurements and diagnostic information.

This prevents a robot from responding dozens of times during a repeated phrase test.

## Conceptual session state

```text
IDLE
  -> STARTING
  -> CALIBRATING
  -> STOPPING
  -> REVIEW
  -> SAVED or DISCARDED
```

## Minimum session data

```json
{
  "session": "pg_001",
  "phrase": "JR BOT",
  "interval_ms": 2000,
  "samples": []
}
```

## Persistence

The test function should operate in RAM first. Only an explicit save action may modify persistent configuration. Incomplete writes or invalid configuration must not compromise JrBot startup.

## Future phases

- Multiple phrases per intent.
- Comparison between profiles.
- Import/export of settings.
- Calibration history.
- WAV assets associated with `say`/`play`.
- Visual Flow editor.
- Integration with `capabilities()`.
