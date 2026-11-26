<div align="center">

<img src="docs/assets/readme/jrbot-logo.svg" alt="JrBot logo" width="360">

# JrBot

**Open-source Physical AI Robot powered by ESP32-S3**  
See. Hear. Speak. Connect. Express.

<img src="https://img.shields.io/github/license/JuniorNarciso26/JrRobot" alt="MIT License">
<img src="https://img.shields.io/badge/ESP32--S3-ESP--IDF-red" alt="ESP32-S3">
<img src="https://img.shields.io/badge/WebRTC-local%20live-blue" alt="WebRTC">
<img src="https://img.shields.io/badge/version-JrBot__V2.0-brightgreen" alt="Current source version">
<img src="https://img.shields.io/github/stars/JuniorNarciso26/JrRobot?style=social" alt="GitHub stars">

[Website](https://jrbot.com.br) ·
[Build your own](#build-your-own-jrbot) ·
[JrSkill Network](#jrskill-network--the-jrbot-skills-ecosystem) ·
[Documentation](docs/README.md) ·
[Roadmap](docs/01-history-roadmap/ROADMAP.md) ·
[Contribute](CONTRIBUTING.md)

</div>

<p align="center">
  <img src="docs/assets/readme/jrbot-demo.gif" alt="JrBot physical robot demo" width="360">
</p>

## AI is leaving the screen.

[Watch JrBot in action — Instagram Reel](https://www.instagram.com/reel/Ddpiov9N8Ts/?obrf=MWN5Mmw0ZGZzazk4Zg==)

Most AI assistants live inside a browser, phone or computer. **JrBot explores what happens when AI has a physical presence.**

JrBot is an open-source ESP32-S3 robot with a camera, microphone, speaker, expressive OLED face, local connectivity and real-time audio/video. Its hardware foundation supports future voice, intelligence and portable Skills.

> **JrBot was not created only to answer. It was created to be there.**

## What JrBot does

| Capability | Implemented features |
| --- | --- |
| **See** | OV5640 photos, browser preview and Live JPEG video; QVGA/VGA/SVGA, image quality and FPS controls |
| **Hear** | MS3625 I2S recording, selectable duration, WAV playback/download and bidirectional WebRTC audio |
| **Speak** | MAX98357A output, speaker playback, audio received from phone/computer, volume and diagnostics |
| **Express** | SSD1306 OLED face with selectable expressions and active-expression status |
| **Connect** | Wi-Fi (DHCP, phone setup and up to four saved networks), automatic saved-profile failover and strong-signal AP selection, bounded idle-only low-RSSI roaming, Wi-Fi dashboard (SSID, IP, RSSI and channel), manual network scan, profile selection/deletion, `jrbot.local` on compatible local networks, local HTTPS and WebRTC |
| **Interact** | Direct browser control on the local network, including Live with independent audio-direction mute |

### JrBot V2.0 — Portuguese offline voice recognition (November 22, 2026)

**JrBot V2.0 begins the V2 voice generation while retaining the established V1 functionality**: Wi-Fi provisioning and diagnostics, connected-mode network dashboard, camera OV5640, OLED expressions, microphone and speaker, local browser controls, WebRTC Live and audio Mesa Mix, and the JrSkill Network.

This first accepted V2.0 milestone integrates **Espressif ESP-SR 2.6.2** and **WakeNet10 `wn10_pt_olaesp`** on the ESP32-S3 HW04. After activating **Autonomous** in the main panel or on the mobile web interface at `jrbot.local`, saying **“Olá ESP”** triggers a short local beep; the OLED then shows **Love for 3 seconds**, automatically returning to Neutral. The wake-word detector resumes listening without requiring an internet connection. The mobile page provides a synchronized on/off sliding switch.

**Owner validation:** WakeNet, beep, Love expression, local controls and coexistence with the prior features were tested and approved on physical hardware. This release starts V2; **free-form speech recognition, custom “Olá JrBot” wake word and full Portuguese TTS are future work**, not capabilities claimed for V2.0.

See [V2 voice integration](docs/04-firmware-development/ESP_SR_OLA_ESP_AUTONOMO.md), [future custom “Olá JrBot” WakeNet plan](docs/04-firmware-development/OLA_JRBOT_CUSTOM_WAKENET.md), [Changelog](docs/01-history-roadmap/CHANGELOG.md), and [Roadmap](docs/01-history-roadmap/ROADMAP.md).

### Develop V2.1.9D_TTS_APP_STREAM — text from the app (November 26, 2026)

Install **develop** with INSTALAR.bat (normal build/flash). In the robot's HTTPS
panel, open **Voz**, enable the synthesizer, save Lite S / speed 1.0 / volume 75 /
gain 6 dB, paste a text and press **Fazer o JrBot falar**. Stop Live and Autonomous
for manual speech. Test the standard phrase first, then a longer multi-sentence
text (up to 1200 UTF-8 bytes / 60 seconds of audio, subject to available memory).

All manual texts now use upstream normalization/sentence splitting and native
PCM callbacks. A producer publishes audio to a bounded PSRAM buffer while a
separate player feeds one I2S channel. Start after about 8 seconds of generated
audio plus a 2-second hold, or after generation finishes for a short utterance.
No hard-coded text split remains. This experiment uses fixed prebuffering, not
the reference firmware's adaptive RTF policy or a reusable ring for unlimited
speech. Initial buffering may change start latency; hardware measurements are
still required. The panel displays **Esperas de áudio**, plus duration/RTF.
If memory or generation fails after playback starts, a prefix may already have
played and the panel reports the failure. Wake reply caching remains separate.

### HTTPS timing investigation (develop, November 26, 2026)

The same benchmark command now logs `HTTPS_PHASES` for every request: DNS/name
resolution, TCP connect, TLS handshake, HTTP request/response-header wait, body
read and total. It retains urllib's original fresh-connection behavior, HTTPS
certificate policy and no POST retries. No firmware change/reflash is required
for this diagnostic; update the host tools before repeating the benchmark.
The 2-second audio hold remains unchanged. Timing data will distinguish a slow
local hostname lookup from TLS/connection or request handling; it does not yet
prove which caused the observed 5–15-second ACK delay. A configured HTTPS proxy
tunnel is flagged, with TLS and proxy CONNECT time combined in that case.

### Develop V2.1.8D_TTS_TWO_BLOCK_2S — fixed split experiment (November 26, 2026)

Install **develop** using INSTALAR.bat, then run the same
`python tools/tts_benchmark.py https://jrbot.local --insecure-local` command.
Only the exact canonical benchmark text uses this experiment: split after
“cada vez melhor.”, render block 1, launch playback with a 2-second initial hold,
and synthesize block 2 while the player waits/plays. Other texts and the wake
reply retain their existing paths. One I2S channel stays open across both blocks,
with the existing 100-ms sentence gap preserved at the split.

Capture the Serial log: `JR_TTS_TWO_BLOCK` records each block's generation/audio
duration; `JR_TTS_TWO_BOUNDARY` records readiness margin (positive = ready early)
and whether the player waited. These are software submission times, not acoustic
measurements. A late second block waits and is reported, rather than dropping
text. This is a two-block experiment, not automatic streaming of arbitrary texts.
The 2-second hold is a test constant, not a guaranteed safe value. Full ESP-IDF
build and physical timing/continuity validation remain pending.

### Develop V2.1.6D_TTS_ACCEL — optimized implementation selection (November 25, 2026)

The owner's existing ELF selected **WEAK** `vtts_dot_s8s8`, `vtts_dots_s8s8` and
`vtts_par_run`, with no `vtts_s3_kernel`: the portable scalar/single-core defaults
were linked instead of the available optimized S3/FreeRTOS implementations.

The S3 build now omits `vtts_kernels_ref.c`, so missing optimized implementations
fail linkage. A final ELF check requires batched dots and the parallel dispatcher
to be GLOBAL/FUNC and confirms the S3 kernel object. The single-dot function may
be absent after optimization and unused-section removal; when present, it must
also be GLOBAL/FUNC. The installer repeats this check before
success/flash, including incremental builds. Expected output:
`[OK] JR_TTS_ACCELERATION_CHECK: SIMD S3 + parallel GLOBAL`.

**This round changes implementation selection only.** Complete buffered playback,
manual task priority, cache and memory policies remain as in V2.1.5D to measure
this cause separately. It does not yet restore streaming or promise 170 ms
startup. Host archive-selection/guard tests and existing TTS/wake/installer tests
pass; full ESP-IDF build, the new ELF check on S3 and physical timing remain pending.

Update/install **develop** with INSTALAR.bat, confirm V2.1.6D_TTS_ACCEL and Git SHA,
then rerun the same JR_TTS_LONG_01 command/video. Share the new benchmark log and
build diagnostics JSON. Stable main and the isolated lab are unchanged.

### Comparative voice test — JR_TTS_LONG_01

The owner-supplied isolated HW04 baseline (November 24, 2026) produced 17.90 s of
speech, started playback after 170 ms (firmware timing), and reported zero
underruns with effective RTF 0.322. This is evidence that the isolated engine can
sustain this phrase on the board; it is not an integrated firmware measurement.

`develop` now includes `python tools/tts_benchmark.py https://jrbot.local --insecure-local`
to repeat the **same 344-byte phrase and Lite S settings** through the existing
HTTPS Voice API after Enter. This addition does not change synthesis/playback.
Film Enter through the end of speech and provide the log. See [results and exact
comparison protocol](docs/04-firmware-development/TTS_LONG_BENCHMARK.md).

### Develop correction: V2.1.5D_TTS_BUFFERED (November 24, 2026)

HW04 testing **rejected V2.1.4D progressive playback**: Lite S took 18,729 ms to generate 5,261 ms of audio (RTF 3.559); Quality M took 39,266 ms for 5,099 ms (RTF 7.700). Each logged 14 FIFO starvation episodes, with IDLE0 watchdog warnings. The supplied video also has repeated roughly 2.5-second quiet gaps. Earlier host tests checked sample order, but did not model the real inference/DMA timing.

`develop` now uses **V2.1.5D_TTS_BUFFERED**: pass the complete text to the native frontend and prepare **all PCM before playback**, retaining the approved 12/128 decoder frame configuration. The 64/128-byte text cuts and concurrent FIFO player are removed. Manual synthesis runs at idle priority, then returns to priority 5 for playback; the watchdog remains enabled. This prioritizes continuous playback over early onset and does **not** claim faster synthesis.

The text limit remains **1,200 UTF-8 bytes**, with generated PCM bounded to **60 seconds or available contiguous PSRAM minus 256 KiB**, whichever is smaller. If synthesis exceeds that space, it returns an error before playing any prefix; shorten the text. First-audio/total-time diagnostics remain available. `main` stays V2.1.3S at `bab09039`.

**Test:** run INSTALAR.bat → **develop**, confirm version/SHA, then HTTPS → Voz, stop Live and Autonomous, select Lite S, enable/save TTS and replay the same phrase. Host tests now exercise full-text delivery, generation-before-playback, exact sample order/final drain and no partial output on buffer overflow. C syntax, JS and wake checks pass; **full ESP-IDF build and HW04 audio/watchdog/Live verification remain pending**. See [technical details](docs/04-firmware-development/VTTS_DEVELOP_INTEGRATION.md).

### JrBot V2.1.3S_TTS_FAST — approved firmware on `main` (November 24, 2026)

**Owner acceptance:** The ESP32-S3 HW04 ran `JrBot_V2.1.3D_TTS_FAST` (Git `7854de30`) with the offline Brazilian Portuguese vTTS engine (Apache-2.0 `tuanpmt/vtts` v0.11.0), WakeNet10 `wn10_pt_olaesp`, 180 ms beep, Love OLED for 3 seconds, and the short acknowledgement **“Olá! Eu sou o JrBot.”** The user confirmed the second wake reply was fast after caching. Serial evidence shows 74,042 bytes of PCM stored in PSRAM, `JR_TTS_WAKE_CACHE_HIT`, `JR_TTS done source=wake result=ESP_OK`, I2S release and WakeNet listening resumed. The earlier v2.1.2 watchdog alert was not seen in the supplied v2.1.3 log.

**Stable promotion:** The accepted code is promoted to `main` as `JrBot_V2.1.3S_TTS_FAST`. The S name and Git SHA differ from the hardware-tested D candidate only because of release metadata and the branch-specific installer path. `main` and `develop` now build this release with **ESP-IDF 6.1.1**, not the previous 5.5.x toolchain. The old V2.0 source remains in Git history, not the current main.

The **Voz** tab allows manual offline TTS in idle mode (Lite S / Quality M, volume, gain, speed, expression and high-frequency attenuation). Manual speech is OFF after reboot, and the current text input limit is **180 UTF-8 bytes**. When Autonomous is enabled, the fixed wake acknowledgement is prepared asynchronously and cached in PSRAM. Beep and Love are preserved, then the cached phrase is played and microphone listening resumes; on synthesis failure the original beep/face behavior is retained.

**One-click Windows installer:** run `INSTALAR.bat` and choose **main** to update from GitHub, prepare private HTTPS materials (never committed), prepare pinned vTTS model assets, stamp the Git SHA, compile, flash automatically to the selected COM port and launch `PAINEL.bat`. The selected ESP-IDF 6.1.1 environment must already be installed. `INSTALAR.bat build` and `INSTALAR.bat flash` retain their diagnostic modes. Boot / `version` / `status` report `version=JrBot_V2.1.3S_TTS_FAST git=<8-character SHA>`; compare the SHA to the installer after compiling.

**Known limitations:** The supplied HW04 log verified that the HTTPS server started on port 443, but recorded TLS handshake alert `-0x7780`; end-to-end browser access and a fresh full WebRTC/Live regression were not established by that log. Other long-session regression scenarios are not newly claimed. This release does **not** include continuous speech-to-text, full conversation or long-text streaming. The failed `develop` V2.1.4D experiment is superseded by V2.1.5D buffered playback; further latency work requires hardware validation. See [vTTS integration and QA](docs/04-firmware-development/VTTS_DEVELOP_INTEGRATION.md), [Changelog](docs/01-history-roadmap/CHANGELOG.md) and [Roadmap](docs/01-history-roadmap/ROADMAP.md).

### What's new in JrBot V1.8S

**JrBot V1.8S** retains all the capabilities listed above and introduces **JrSkill Network** integration. Users can acquire Skills through the platform, manage their licenses and execute them directly on JrBot.

The initial Creator Studio, Store, licensing and Skill execution flow have been completed and validated. **An expanded platform for developing and publishing Skills is now under development**, offering new possibilities for creators and developers.

**V1.8.1S** improves Wi-Fi setup: JrBot opens its own setup network when it cannot connect at startup, tests new credentials before saving, and retries local `jrbot.local` announcement after an mDNS startup failure. On October 26, 2026, the project owner confirmed that the current physical tests passed and memory testing was completed. This confirmation does not provide new per-scenario logs or memory measurements.

**JrBot V1.8.4S** extends Wi-Fi reliability: automatic selection of a compatible access point with stronger signal at connection time; retry/failover across saved profiles; a one-time automatic reboot during startup recovery before opening phone setup; and lightweight Serial diagnostics. An ESP-IDF native low-RSSI event (threshold **-70 dBm**) may trigger a scan of the current SSID **only while idle**, with a controlled reconnection when a different access point offers at least **12 dB more signal** (minimum **90 seconds** between scans). No continuous scan task runs, and preventative roaming is suppressed during Live/recording. These policies were designed for generic customer networks, not tuned to a particular company.

### JrBot V1.8.4S: More reliable Wi-Fi (inherited by V2.0)

- **Better AP selection:** the ESP-IDF driver checks all Wi-Fi channels and prefers the strongest compatible access point for the selected SSID when connecting, including on reconnection; no fixed BSSID.
- **Recovery across networks:** up to four saved Wi-Fi profiles with bounded retries and automatic fallback, remembering a working alternative. One bounded boot-only restart helps recover when the initial setup times out; captive phone setup remains available.
- **Low-signal protection:** the native ESP-IDF RSSI event at **-70 dBm** triggers a same-SSID scan **only in idle mode**. Switching requires at least **12 dB** improvement over the current access point and scans are rate-limited to **90 seconds** apart. This is application-managed reconnect, not seamless 802.11 roaming. Video/audio sessions are never deliberately interrupted for a preventive scan.
- **Diagnostics:** Serial logs include real connected SSID/BSSID, signal, Wi-Fi disconnection reasons, reconnection attempts, internal RAM/PSRAM samples and failing HTTP routes (without exposing Wi-Fi passwords).
- **Hardware evidence:** in a short ESP32-S3 HW04 test, the robot selected the stronger of two `CPGAS_5G` access points (~-42 versus -82 dBm), remained connected with **zero recorded Wi-Fi disconnects**, and completed a WebRTC session. The low-signal roaming path was **not** triggered in that test. Independent full ESP-IDF build/regression and long-running reliability validation were **not** recorded. HTTPS TLS alerts and WebRTC video-buffer/memory pressure remain known issues.

The ESP32-S3 has **2.4 GHz Wi-Fi only**; an SSID containing “5G” does not indicate a 5 GHz radio link. See [Wi-Fi dashboard and roaming details](docs/04-firmware-development/WIFI_DASHBOARD.md) and [Project status](docs/01-history-roadmap/PROJECT_STATUS.md).

### JrBot V1.8.2S: Mesa Mix for Live audio

**JrBot V1.8.2S** introduced **Mesa Mix · Live**, which lets users adjust WebRTC speaker volume, microphone gain, echo ducking, noise gate and pre-buffer in the robot's web page. Settings apply during the Live and reset when the robot restarts. The owner reports good communication with headphones and improved sound after lowering microphone gain and speaker volume. A delayed return of their own voice remains under investigation; no RAM/PSRAM time series were supplied for this version. See the [Mesa Mix guide](docs/04-firmware-development/LIVE_AUDIO_LAB.md).

### JrBot V1.8.3S: Wi-Fi dashboard after first setup

**JrBot V1.8.3S** is the previous source release. The robot's local page, `https://jrbot.local`, now has **Control · Memory · Wi-Fi** tabs. The Wi-Fi tab shows whether the robot is connected, the current network (SSID), assigned IP, signal strength (RSSI in dBm), Wi-Fi channel and current operating mode. You can refresh the signal on demand or optionally every 10 seconds while the tab is visible.

When the robot is idle, you can scan nearby networks (up to eight displayed), save a new network or update a saved profile, view up to four saved networks, select which one to use next, and **delete a saved network** with confirmation. Changing the selected network does **not** interrupt the active connection immediately: applying it requires a separate, confirmed reboot. Deleting the selected profile warns that first-time setup may be required at the next boot.

The existing first-run phone setup remains available when the robot cannot connect. Network changes use a dedicated **HTTPS-only** local endpoint; the ordinary HTTP command route still rejects Wi-Fi commands. **This panel does not yet implement user authentication and must not be exposed directly to the public Internet.** The new release is integrated in source control; compile/runtime memory and full hardware regression metrics have not been independently verified in this documentation update.

See [Wi-Fi dashboard guide](docs/04-firmware-development/WIFI_DASHBOARD.md), [Installation](docs/04-firmware-development/DEVELOPMENT.md) and [Project status](docs/01-history-roadmap/PROJECT_STATUS.md).

### Current limitations

Live communication currently works on the local network. Remote Internet connectivity and audio/video improvements remain planned for future versions. See [Testing](docs/04-firmware-development/TESTING.md) and [Project status](docs/01-history-roadmap/PROJECT_STATUS.md).

## Build your own JrBot

JrBot is designed so technology enthusiasts can build their own robot using **widely available, off-the-shelf electronic components**.

At its heart is an **ESP32-S3 CAM (N16R8)** board with an **OV5640 camera**. It uses the familiar **MS3625 microphone** and **MAX98357A audio amplifier** for sound, while a small **SSD1306 OLED display** brings its expressions to life.

**Want to build your own JrBot?** Explore the [parts list](docs/02-assembly/BILL_OF_MATERIALS.md), [wiring reference](docs/02-assembly/WIRING.md), and [assembly guide](docs/02-assembly/ASSEMBLY_GUIDE.md).

## JrSkill Network — The JrBot Skills Ecosystem

**JrSkill Network** extends JrBot with a way to discover, acquire and execute new robot abilities, called **Skills**. Users can browse Skills, connect a Solana wallet, manage their licenses and run supported actions on their own JrBot.

### How it works

The ecosystem connects four parts:

1. **Creator Studio** — the initial, validated interface for creating and publishing structured Skills; its developer tools are being expanded.
2. **JrSkill Store** — a place to discover Skills and acquire licenses using a Solana wallet such as Phantom.
3. **Solana + JrBot Cloud** — the licensing, transaction and verification layer, currently validated on **Solana Devnet**.
4. **My Skills on JrBot** — the robot's interface for finding licensed Skills and executing supported actions through its controlled runtime.

~~~text
Create a Skill → Publish to Store → Acquire a license
        → My Skills → Execute on JrBot
~~~

Skills run through approved robot operations rather than unrestricted access to hardware or native code. **The existing creation and publishing flow is operational on Devnet; the expanded developer platform is the next development stage.**

### Developed during Colosseum 2026

**JrSkill Network began as a major JrBot upgrade during the Colosseum / Crypto World's Fair 2026 hackathon** and is now part of **JrBot V1.8S**. During the event, we completed and validated the entire flow on Solana Devnet: creating and publishing Skills, acquiring licenses through the Store and executing a licensed Skill on a physical JrBot. The independent test Skill `jrteste` was successfully executed on the robot.

That first stage is complete. Work now continues on expanding the Skill development and publishing platform. **Devnet validation is not a claim of a live Solana Mainnet marketplace.**

**Want to see how we built JrSkill Network?** Follow the complete [Colosseum 2026 development diary — Kickoff and Day 1–9](docs/06-colosseum-history/HACKATHON_DEVLOG.md), including tests, screenshots and physical validation.

[Explore the JrSkill Store](https://jrbot.com.br/en/store/) · [JrSkill technical documentation](docs/05-jrskill-network/JRSKILL_API.md) · [Colosseum project](https://colosseum.com/arena/projects/jrskill-network)

## Project status and roadmap

**JrBot V2.0** retains the V1.8S ecosystem and earlier capabilities and adds local offline activation via WakeNet10 in Portuguese. **JrBot V1.8S** integrated the existing robot capabilities with **JrSkill Network**. The earlier Solana Devnet flow for acquiring, licensing and executing Skills was validated on its recorded build. **V1.8.1S** added phone-based Wi-Fi setup and mDNS recovery; the project owner reported physical tests OK and memory testing complete on October 26, 2026. **V1.8.2S** added the Live audio Mesa Mix, promoted from the tested `AUDIO_LAB_01` branch with the delayed voice return documented as a known limitation. **V1.8.3S** added the connected-mode Wi-Fi dashboard, manual signal/network inspection and saved-network management, including selection and confirmed deletion. **V1.8.4S** adds the stronger-AP selection, saved-profile recovery, limited idle-only roaming and diagnostic logging described above.

| Project | Status |
| --- | --- |
| **JrBot V1.8S** | Integrated robot and JrSkill capabilities; preparing the official release |
| **JrSkill development platform** | Expanding tools for creators to develop and publish new Skills |
| **JrBot V2.0** | First voice milestone delivered: offline “Olá ESP” wake word, beep, Love face for 3s, and autonomous mobile/panel controls |
| **JrBot V3** | Programmatic control and Runtime API evolution — planned |
| **JrBrain** | Memory, personality, context and AI — future vision |

Explore the full [project roadmap](docs/01-history-roadmap/ROADMAP.md). The [Colosseum 2026 development diary](docs/06-colosseum-history/HACKATHON_DEVLOG.md) preserves the history of the JrSkill upgrade.

## Contributing and documentation

JrBot welcomes contributions in ESP32-S3 firmware, hardware, audio/WebRTC, camera, frontend, AI, Solana, testing and documentation. You do not need to own a complete robot to contribute.

Start with the [Contribution Guide](CONTRIBUTING.md) and [open Issues](https://github.com/JuniorNarciso26/JrRobot/issues). Describe the change, evidence and any missing physical tests in a Pull Request.

**Documentation:** [Index](docs/README.md) · [Wi-Fi Dashboard](docs/04-firmware-development/WIFI_DASHBOARD.md) · [Architecture](docs/04-firmware-development/ARCHITECTURE.md) · [Development](docs/04-firmware-development/DEVELOPMENT.md) · [Testing](docs/04-firmware-development/TESTING.md) · [Runtime API](docs/04-firmware-development/API_RUNTIME.md) · [Changelog](docs/01-history-roadmap/CHANGELOG.md)

## License

Original repository content is released under the [MIT License](LICENSE), unless explicitly stated otherwise. Third-party components retain their own licenses. The MIT License does not grant rights to the **JrBot** trademark or authorize presenting derivative products as official JrBot products.

## Vision

**JrBot connects physical presence with a future ecosystem of portable abilities.** The goal is a robot that can listen, perceive, respond and gain new Skills without losing control of its hardware.

<p align="center">
  <strong>JrBot</strong><br>
  Listen. Understand. Respond. React. Express.<br><br>
  <em>Project by JrTk Invest</em>
</p>

---

<p align="center">
  If you want to follow the evolution of open-source physical AI, consider starring the repository.
</p>
