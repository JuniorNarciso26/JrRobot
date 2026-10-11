# JrBot Testing and Validation

**A successful build, HTTP response or log entry is not proof of physical behavior.** Every report should state the evidence level.

## Evidence levels

| Level | What is verified |
| --- | --- |
| **Planned** | Design or documentation only |
| **Implemented** | Code exists, but build is not confirmed |
| **Build verified** | Firmware compiles for the intended target |
| **Bench / simulated** | Logic exercised with a mock, simulator or diagnostic |
| **Physically validated** | Behavior directly observed on real hardware |

## What to observe

| Feature | Required distinction |
| --- | --- |
| **OLED** | A successful `face` response is not sufficient; confirm the visible expression |
| **Microphone** | Driver success does not prove audible captured audio; listen to the recording |
| **Speaker** | `ESP_OK` does not prove sound was heard; record actual playback |
| **Camera** | A valid JPEG does not establish correct focus, exposure or visual quality |
| **Live WebRTC** | Observe video and both audio directions on the local network |
| **Voice / wake word** | Simulated `brain_test` is not acoustic recognition; record spoken positive/negative trials |
| **JrSkill** | A Devnet transaction or `result=ok` is not, on its own, proof that physical actions occurred |

## Voice experiment checklist

For a real voice test, record the board/build and speech model; ensure the microphone is active; repeat the target phrase at different distances/volumes; include unrelated speech, noise and silence; record recognizer scores; confirm behavior and listening recovery; save logs; check for watchdogs or reboots.

The planned [Playground](PLAYGROUND.md) should distinguish positive spoken trials from background/negative samples. Acceptance criteria such as "8 of 10 positive trials" must be described as **project targets**, never guarantees of the speech-recognition library.

## Minimum test report

```text
Date:
Firmware build / branch / commit:
Hardware:
Test procedure:
Expected result:
Observed result:
Evidence (logs, images, video):
Evidence level:
Known issues:
```

For current release-readiness work, use [Project Status](../01-history-roadmap/PROJECT_STATUS.md) and the [Roadmap](../01-history-roadmap/ROADMAP.md).
