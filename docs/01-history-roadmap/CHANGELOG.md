# JrBot Changelog — The Journey from Prototype to Skills Ecosystem

JrBot did not start as a completed robot. It grew from **a simple expressive OLED face**, through local control, camera, audio and real-time communication, into a physical robot capable of executing licensed Skills.

This is the **chronological record of what was built and tested**. Early experiments, firmware candidates and officially promoted versions are distinguished below. The dates and milestones are based on repository commits and recorded validation; they are not invented product-release dates.

For upcoming work, see the [Roadmap](ROADMAP.md). For what is available now, see [Project Status](PROJECT_STATUS.md).

## November 26, 2026 — JrBot_V2.1 promoted to main

- Owner explicitly approved app speech and the greeting/app name pronunciation, then requested promotion of develop to main. Promote the approved runtime unchanged from bbc9e3e; preserve the simple JrBot_V2.1 display label and Git/build identification.
- Release native-block manual speech with concurrent producer/player, fixed initial reserve/hold, bounded input/PCM, optimized S3 kernels, cached wake greeting, whole-name pronunciation alias and updated Voz feedback. Include the tested installer/update and SDK validation corrections and diagnostic tools.
- Update README/version badge, documentation index, Project Status, Roadmap and integration guide. Record measured HW04 speech evidence and owner pronunciation approval without claiming a new complete regression of inherited functions.
- Keep future development on develop. No new implementation is started as part of this release; the next phase's scope remains to be selected.

## November 26, 2026 — JrBot pronunciation experiment (develop)

- Add a pronunciation-only whole-name alias for case-insensitive JrBot/JRBot/jrbot: djêi ár bót, targeting the owner's English-letter Jay Ar Bot pronunciation while retaining the pt-BR frontend. The exact sound requires hardware listening; the visible name and JrBot_V2.1 label stay unchanged.
- Apply immediately before native text normalization for both app/manual speech and wake cache preparation/uncached fallback. Cached wake playback remains unchanged and is regenerated at boot. No upstream files, gain, streaming or buffering changes.
- Preserve UTF-8 text and embedded names such as meuJrBot, JrBotão, JrBot2 and JrBot_nome. Bound the expanded spoken text to the existing 1200-byte limit; fail on overflow/allocation error rather than silently truncating. Tests exercise manual synthesis input, unchanged visible job text, wake cache preparation/hit, whole-word/case/UTF-8 boundaries and capacity limits.

## November 26, 2026 — JrBot_V2.1 display name and owner approval

- Simplify firmware/version.txt and JR_APP_VERSION to JrBot_V2.1 as requested. Preserve the build stamp and generated Git commit for precise build identification. No inference, playback, installer or settings changes; main is unchanged.
- Owner approved the app streaming result. Supplied JrBot-painel-log (58).txt records two manual utterances completing ESP_OK: 344-byte/7-segment text, 17,597 ms PCM, 14,225 ms synthesis, first I2S at 9760 ms; then 141-byte/4-segment text, 7173 ms PCM, 5914 ms synthesis, first I2S at 6120 ms. No STREAM_STARVED entries appear in this supplied log. This validates these speech samples, not all other hardware functions.

## November 26, 2026 — V2.1.9D_TTS_APP_STREAM (develop experiment)

- Enable native PCM-window streaming for all manual text submitted through /voice and the app's Voz tab. Remove canonical-text comparison and two predetermined halves. Keep upstream text normalization/segmentation, first/cap window sizes, 100-ms sentence gap, voice processing and HTTPS unchanged.
- A single inference producer publishes a fully written immutable prefix of the existing bounded PCM allocation; a separate player waits for 8 seconds of generated audio plus a 2-second initial hold, then consumes published samples through one I2S session. Short complete utterances skip the extra hold. This is fixed prebuffering, not upstream's adaptive RTF policy or unbounded-duration streaming. Keep 1200-byte input and at most 60 seconds of PCM, retaining the PSRAM reserve.
- Synchronize availability/completion/error publication, count software starvation episodes, stop inference at the next PCM callback after playback failure, and join the player before freeing any resources. Keep cached wake response and uncached wake fallback separate.
- Update the app with streaming feedback, starvation count, completion metrics and accurate partial-playback error wording. Serialize timer polls to avoid overlapping requests.
- Host regression covers arbitrary UTF-8 text, early playback before inference completion, native segment gaps/sample order, short utterances, late producer, allocation/overflow/inference/I2S/task failures, and real wake cache preparation/hit. C host and JS syntax validation pass. Full ESP-IDF build, watchdog, acoustic continuity/volume and Live/Autonomous coexistence require HW04 testing. Main remains unchanged.

## November 26, 2026 — HTTPS phase diagnostics (develop host tools)

- Investigation confirms urllib's AbstractHTTPHandler.do_open forces Connection: close. Existing benchmark polls therefore open a fresh TCP/TLS connection each time; repeated ESP HTTPS handshake logs are consistent with that behavior. This alone does not identify whether DNS/mDNS, TCP, TLS or HTTP response handling caused the measured 14853.2/5138.0-ms ACKs.
- Add per-request HTTPS_PHASES timing to the same benchmark: OS hostname resolution, TCP address attempts, TLS (or proxy CONNECT plus TLS), HTTP request/response-header interval, body read and total. Log failures as well as successes, plus protocol jr-tts-https-phases-v1 and request action/sequence. Preserve redirects rejection, certificate verification policy, fresh connections and no HTTP/POST retries. No hostname caching, TLS setting changes or keep-alive optimization is introduced in this diagnostic round.
- Five real local TLS regression tests cover phase reporting, existing Connection: close semantics, uncertain-delivery single submission, redirects rejection, certificate verification and DNS failure. All 15 TTS tests pass. Hardware transport timing awaits the next owner-provided log. Firmware, sound processing and 2-second hold are unchanged; no reflash is required.

## November 26, 2026 — ESP-IDF environment validation (develop installer)

- Before export.bat, validate the selected ESP-IDF 6.1 SDK using version.cmake and esp_idf_version.h, reject inconsistent version.txt metadata and mismatched Python environment markers, and verify the running interpreter belongs to the selected venv.
- Use the official offline check-python-dependencies action. Only after it passes, exclusively create a missing venv/idf_version.txt containing the official major.minor family. Existing mismatched identifiers are preserved and rejected; no tools, packages or Git tags are installed/fabricated.
- Replace the installer's idf.py --version/findstr pipeline with a source-version check whose real exit code is retained. This avoids its noisy git describe failure on tagless SDK distributions. Third-party build commands may still emit their own tag diagnostics; SDK sources/Git metadata are not modified.
- Seven new offline regression tests cover missing/wrong markers, dependency failure, interpreter mismatch, inconsistent SDK metadata, version CLI exit status and installer integration. Native Windows export/build still needs validation on the user's PC. Firmware/TTS settings remain unchanged.

## November 26, 2026 — V2.1.8D_TTS_TWO_BLOCK_2S (develop experiment)

- Reduce only the canonical two-block benchmark's initial hold from 4000 to 2000 ms. Keep the split, voice settings, inference/player implementation, memory policy, I2S ownership and HTTPS unchanged.
- Previous HW04 measurement (V2.1.7D, 04737ccf): block 1 generated in 6065 ms for 7382 ms PCM; block 2 generated in 8128 ms for 10214 ms PCM. The second block was ready 3255 ms before the first finished I2S submission, waited=0; command completed ESP_OK. Total synthesis 14194 ms; first I2S 10259 ms; device total 27908 ms.
- With identical timings, removing 2 seconds would leave about 1255 ms margin. This is an estimate, not a measured result for V2.1.8D. Previous HTTPS ACK took 15100.7 ms; transport investigation is separate from this controlled hold experiment. Physical continuity/watchdog testing remains required.

## November 26, 2026 — V2.1.7D_TTS_TWO_BLOCK (develop experiment)

- The owner approved audio quality of V2.1.6D, git 1800282b. The supplied benchmark reports 21,390 ms synthesis, 22,194 ms first I2S submission and 17,597 ms PCM; the uploaded video has no audio track. This approval covers long-text sound, not all hardware features.
- For the canonical benchmark text only, split after “cada vez melhor.” and generate the second half concurrently with a dedicated playback task. Wait 4 seconds after block 1 is generated before playback, allowing block 2 a head start. Preserve the model/arena, bounded PCM allocation, single I2S session, sample order, boundary silence and final DMA drain. No overlap of two neural inference calls.
- Log per-block synthesis/audio duration and signed readiness margin at the boundary. Count a boundary wait in last_underruns (a software starvation indicator, not an acoustic measurement). Join playback even if generation or I2S fails before releasing PCM, model or I2S ownership.
- Host tests exercise overlap, exact PCM order/boundary gap, early and late second blocks, second-generation failure, playback failure and task-creation failure, plus prior buffered/wake paths. ESP-IDF build and physical validation remain pending. Main remains unchanged.

## November 26, 2026 — TTS ELF validation correction (develop)

- Keep the main installer's runner/menu/update/build/flash flow. Before syncing, automatically stash local tracked edits and untracked files instead of requiring manual Git cleanup. Do not reapply that backup over the downloaded firmware. Ignored build/model files are left alone. Existing branch-tracking and pre-flash acceleration checks remain in place.
- Owner's build 09b64712 contains GLOBAL batched dots (322 bytes), parallel dispatcher (158 bytes) and S3 kernel object (4 bytes), with the single-dot symbol absent. TTS units use -O2 after -Og. The original guard rejected this ELF solely for the absent single-dot symbol.
- Allow an absent single-dot entry point, which can be eliminated when unused. Still reject a weak or invalid single-dot symbol when present; batched dots, parallel dispatcher, Xtensa target and S3 kernel object remain required. Generic weak fallback sources stay excluded.
- Firmware algorithms/settings and version remain unchanged. Host validation does not establish physical timing or audio quality.

## November 25, 2026 — V2.1.6D_TTS_ACCEL (develop)

- Owner's existing build diagnostics returned WEAK/FUNC for vtts_dot_s8s8 (45 bytes), vtts_dots_s8s8 (35 bytes) and vtts_par_run (14 bytes), with vtts_s3_kernel absent. This identifies portable scalar/single-core implementations in that ELF; the full JSON/configuration has not yet been supplied.
- ESP32-S3 builds omit vtts_kernels_ref.c. Its three weak fallback definitions can satisfy archive references before optimized objects are extracted. Omitting them makes missing S3/FreeRTOS implementations a link error, independent of archive ordering. Upstream sources remain unmodified.
- Add post-link ELF validation via the final ESP-IDF EXECUTABLE target, and repeat validation in INSTALAR.bat after any inventory rebuild and before reporting build success or flashing. Require Xtensa, all three functions GLOBAL/FUNC with nonzero size, and a GLOBAL S3 kernel object.
- Only this implementation-selection cause changes: retain buffered playback, idle-priority manual synthesis and memory/cache settings for a controlled timing comparison. Performance improvement and continuous streaming are not yet physically demonstrated by this patch.
- Two new regression tests exercise actual host archive selection/missing-port link failure and guard rejection of weak/missing symbols. Ten TTS, eleven installer and six wake tests passed; Python syntax passed. Full ESP-IDF/CMake/S3 build remains pending (toolchain/CMake unavailable here).
- Stable main and isolated TTS lab branches are unchanged.

## November 24, 2026 — Integrated long-text measurement and read-only build diagnostics

- Owner's V2.1.5D/b0749963 logs confirm 133,291 ms first I2S write, 133,097 ms synthesis for 17,597 ms PCM (RTF 7.564), 1,427.4 ms HTTPS ACK and 153,051.8 ms host completion. Another panel run took 134,456 ms synthesis; the submitted comparison was the second utterance, not a cold first utterance.
- Memory snapshots show ~2.86 MiB PSRAM free during synthesis and ~7.90 MiB after; no allocation failure/reset in these samples. Record exact comparison and limitations in TTS_LONG_BENCHMARK.md.
- Add tools/tts_build_diagnostics.py to inspect the existing ELF's strong/weak vTTS symbols, selected effective configuration, optimization flags and local Git/vendor stamps without compiling or touching the device. Only whitelisted fields are exported, not full configuration/certificates.
- Three parser/filtering tests and five existing TTS tests pass. Real build collection pending. Firmware algorithms/settings, installer and version remain unchanged.

## November 24, 2026 — JR_TTS_LONG_01 isolated baseline and integrated comparison

- Owner supplied the fixed-phrase isolated test: 344-byte canonical text/hash matched, Lite S settings confirmed, 17.90 s PCM, 96 ms first block, 170 ms playback start, 0 underruns, effective RTF 0.322, 18,085.1 ms host completion. These are isolated metrics; the video does not show the Enter key for exact acoustic latency.
- SAID's 14.029 s wall time includes FIFO backpressure; it is not startup latency. Record the net RTF and producer-wait distinction in the benchmark protocol.
- Add a host HTTPS sender for develop with identical text/settings, Enter trigger, device metrics and local result logs. Validate available/idle state, disable redirects and restrict to local HTTPS origins, and never retry speech POST after uncertain delivery.
- No firmware algorithm/version or installer changes. Integrated test still needs user installation, HTTPS execution and video. Four sender tests, existing buffered synthesis host test and Python syntax checks pass.

## November 24, 2026 — Installer branch tracking correction

- Persist the wildcard remote.origin.fetch mapping before fetching all branches. An explicit fetch alone left single-branch clones unable to create tracking branches, producing “starting point origin/test/vtts-pt-br-hw04 is not a branch”.
- Switch from the verified full remote ref, repair upstream metadata for existing local branches, and keep fast-forward-only updates/local-change protection. No firmware/version changes.
- Two tests reproduce the original failure in real local single-branch clones, then verify switching/tracking/pull and existing branches with duplicate fetch mappings. Windows cmd.exe execution still needs user verification.
- Bootstrap an existing failing develop clone with `git pull --ff-only origin develop`, then reopen INSTALAR.bat. Its old temporary runner cannot pick up its own fix midway through an already running update.

## November 24, 2026 — V2.1.5D_TTS_BUFFERED (develop correction)

- HW04 log (53) rejects V2.1.4D: S 18,729 ms synthesis / 5,261 ms audio, M 39,266 / 5,099 ms; 14 FIFO starvation episodes per utterance, repeated IDLE0 watchdog warnings. Video audio analysis shows repeated approximately 2.5-second quiet gaps; it does not identify which specific words were lost.
- Remove sample-level concurrent playback and artificial 64/128-byte text slicing. Restore complete native-frontend input and the accepted 12/128-frame decoder strategy; play only once every PCM sample is ready. This restores the buffered approach at the cost of waiting for complete synthesis.
- Retain 1,200-byte input limit; PCM capacity is bounded by 60 seconds and largest available PSRAM block minus 256 KiB. If generation exceeds that cap, return ESP_ERR_INVALID_SIZE before any audio, with panel guidance to shorten the text. No partial-prefix playback on synthesis failure.
- Manual worker runs at idle priority through the text frontend and synthesis, allowing IDLE0 execution even between PCM callbacks; raise to priority 5 only for playback. Keep watchdog enabled and do not claim it fixed until hardware retest.
- Retain first-audio and total-time metrics, and drain more zero frames than the configured DMA depth before disabling I2S. Wake acknowledgement still uses its cached PCM, beep/Love and microphone hand-back sequence.
- Host tests now assert full text reaches frontend once, I2S writes start after final synthesis callback, exact PCM/final drain, no output on overflow, allocation and I2S errors, and cached PCM bounds. C syntax with pinned upstream headers/vTTS-disabled, JS syntax and wake guards pass. ESP-IDF build and HW04 verification remain pending.
- `main` remains unchanged at bab09039. This is a continuity correction, not a proven latency optimization.

## November 24, 2026 — V2.1.4D_TTS_STREAM (develop experiment)

- Manual input increases from 180 to 1,200 UTF-8 bytes. Text parts target 64 bytes initially, then 128 bytes, retaining word boundaries and complete UTF-8 characters; a longer single word stays intact.
- Manual PCM is consumed by a separate I2S task while inference continues, through a fixed 32 KiB PSRAM FIFO. Decoder first block is 12 frames, later blocks grow up to 32; the model and smaller streaming arena are reused across parts.
- Manual audio is bounded to 300 seconds of generated samples; errors may follow an already played prefix. Busy/I2S ownership persists until the player finishes and releases its physical channel.
- `/voice` adds `last_first_audio_ms`, `last_total_ms`, `last_underruns`; the panel shows startup time and generation pauses. Synthesis time/RTF exclude time spent sending to the FIFO. First-audio time is request-to-first-successful-I2S-write, not measured acoustic onset; underruns count FIFO receive timeouts after playback starts.
- HTTPS JSON body limit accommodates escaped long text, allocated on heap; speech's decoded limit remains enforced in firmware. The wake cache renderer retains its original 12/128 frame settings and 12-second clip bound, and beep/Love/wake ordering is retained.
- Host tests cover actual player/FIFO code, Unicode boundaries, playback before end-of-input, ordered samples across FIFO wrap, final drain, I2S failure, allocation/task failure and cached PCM bounds. Existing wake/installer tests and JS syntax checks pass; C syntax checked with pinned vTTS headers and vTTS disabled. Full ESP-IDF build and HW04 measurements remain pending (toolchain unavailable here). No speedup or Live regression result claimed yet.
- Only `develop` changes; stable `main` remains V2.1.3S at `bab09039`.

## November 24, 2026 — JrBot V2.1.3S_TTS_FAST (owner-approved promotion to main)

**Release identity:** `JrBot_V2.1.3S_TTS_FAST` uses ESP-IDF **6.1.1** and upgrades the previous V2.0 ESP-IDF 5.5.x baseline; source and models from `develop` are promoted together. The exact HW04 candidate was `JrBot_V2.1.3D_TTS_FAST` (commit `7854de30`); the stable suffix and installer selection change the resulting build ID without changing the accepted speech algorithms.

- Offline pt-BR TTS based on vTTS v0.11.0 (Apache-2.0), with manual **Voz** tab, Lite/Quality models and configurable volume/voice tuning.
- WakeNet10 still detects **“Olá ESP”**, plays **880 Hz / 180 ms** beep, shows **Love** on OLED for 3 seconds, then says **“Olá! Eu sou o JrBot.”** and resumes listening.
- Fixed wake reply is pre-synthesized at Autonomous activation and cached as approximately **74,042 bytes of PCM in PSRAM**. Owner reports the second invocation was noticeably fast; logs show `JR_TTS_WAKE_CACHE_HIT` and successful mic/I2S recovery.
- The prior experimental TTS triggered a CPU0 watchdog warning during synthesis (5,808 ms to produce 1,678 ms of audio); the v2.1.3 sample log had no such warning. This is not a claim of exhaustive long-term stability.
- The one-click installer now uses **ESP-IDF 6.1.1 for both main and develop**, prepares private HTTPS/vTTS resources, stamps Git SHA, and flashes automatically to the selected COM after a successful build.

**Evidence scope and known risks:** The supplied HW04 log confirms server initialization on HTTPS port 443, camera detection, Wi-Fi reconnection and stable wake playback. It also recorded TLS handshake alert `-0x7780` on some client attempts; fully successful browser TLS and comprehensive Live/WebRTC regression were not separately demonstrated in these logs. This was an explicit owner-accepted promotion. **Long-text streaming is not included**; the next implementation will be developed on `develop` and verified independently.

## November 22, 2026 — JrBot V2.0 begins: first offline voice milestone

**Owner-confirmed hardware validation and release acceptance.** V2.0 builds on all
previous V1 capabilities through V1.8.4S and introduces
**ESP-SR 2.6.2 WakeNet10 `wn10_pt_olaesp`** for Portuguese local wake-word
detection on the ESP32-S3 HW04.

- The existing panel's **Autonomous** control now activates the validated
  **“Olá ESP”** model instead of the experimental English MultiNet path.
- On wake detection, the robot gives a short **880 Hz / 180 ms beep**, then
  shows **Love** on the OLED for **3 seconds** and automatically returns
  to **Neutral** without blocking recognition.
- `jrbot.local` on the phone gains a sliding switch for autonomous on/off,
  with real state synchronization from the robot.
- The existing hardware and software functions remain available: OLED,
  camera, microphone, MAX98357A sound, network management, Wi-Fi roaming,
  WebRTC Live/Mesa Mix, local control and JrSkill Network.
- The owner reports that the Portuguese model did not materially burden the
  project and that the existing features continued working.

**Scope:** this is the first **V2.0** milestone, not continuous speech-to-text,
free-form Portuguese TTS, voice conversation or custom “Olá JrBot” yet.
The release is recorded as owner-tested; this entry does not invent new
independent compile logs, RAM measurements or full regression data.
[Implementation and testing](../04-firmware-development/ESP_SR_OLA_ESP_AUTONOMO.md).

## August 2026 — The project takes shape

### August 1 — First repository record

The first recorded commit established the `JrRobot` repository. At this point, GitHub history establishes the **start of the recorded project**, not a completed firmware or a physical release.

[Initial project commit](https://github.com/JuniorNarciso26/JrRobot/commit/fa82fe81eca555d70f2494c5dd5e30659079be92)

### August 31 — JrBot gets its first face

The first documented functional firmware brought expressive eyes to an OLED connected to an **ESP32-S3**.

- Implemented an I2C OLED face with expressions controlled through Serial/USB.
- Added I2C scanning, terminal commands, and Windows/Linux scripts to build and flash the board.
- Confirmed a **physical OLED test**, with the display responding at I2C address `0x3C`.
- Established the initial division between the embedded firmware and a computer-based control interface.

This first hardware experiment used **GPIO8/GPIO9 for the OLED**. Those were *historical prototype connections*, not the final HW04 pin map.

[First physically tested face](https://github.com/JuniorNarciso26/JrRobot/commit/4da38de4251303ea840a5fa4b92a6b75d30564cf)

## September 2026 — From a face to an interactive robot

### September 1 — A visual control panel and Wi-Fi

JrBot advanced beyond typing commands into a Serial console.

- Expanded the OLED expression firmware and introduced a **local HTML control panel**.
- Added a Wi-Fi panel served by the ESP32, opening the path to browser-based interaction.
- Began packaging the firmware and simplifying installation.

An early commit called its OLED revision "face OLED v2". That was a **prototype revision**, not the later *JrBot V2 product line* for voice control.

[OLED panel milestone](https://github.com/JuniorNarciso26/JrRobot/commit/0c7fd0d4e84f1a4cf47dce4b28da5f78cf9aa1f6) · [Wi-Fi panel milestone](https://github.com/JuniorNarciso26/JrRobot/commit/170ed12dce6f2aa3592d1e6493af61d2cc614609)

### September 3–6 — Camera, sound and reference hardware

The robot began gaining the hardware interfaces needed to see and communicate.

- Added panel-driven camera capture and tests for the **MAX98357A** audio amplifier.
- Refactored the firmware toward modular components.
- Improved installation, diagnostics, Serial-port selection, and JPEG capture over Wi-Fi.
- Revised the electrical design through **HW03 and HW04**, confirming the OV5640 and the shared I2S audio signals.

The HW04 audio reference established **GPIO21 for BCLK, GPIO47 for WS, GPIO41 for microphone SD, and GPIO42 for amplifier DIN**. This September 6 milestone confirmed wiring, but **the microphone's RX driver had not yet been implemented in that revision**. Later versions added and validated microphone functions.

[Audio test](https://github.com/JuniorNarciso26/JrRobot/commit/6734fde4ec) · [HW04 wiring confirmation](https://github.com/JuniorNarciso26/JrRobot/commit/71f8adc172)

### September 7–8 — Early autonomy experiments and a safer Runtime API

Two important foundations appeared:

- **Local voice experiments:** autonomy controls, WakeNet/MultiNet recognition experiments, and local response behavior. These were development checkpoints, **not the release of a finished offline voice product**.
- **Runtime API v1.1:** a controlled interface for discovering capabilities, reading status and invoking a registered `face` action. It supported local HTTP/Serial control while avoiding arbitrary native-code or GPIO execution.

The Runtime API v1.1 line was integrated and promoted to `main` on **September 8**, alongside the I2S Philips framing correction.

[Runtime API 1.1 promotion](https://github.com/JuniorNarciso26/JrRobot/commit/76e0ab48b6089e8397c6bf304c242f85352253fb)

### September 13 — The first dedicated V1 control experience

The V1 workstream focused on controlling the robot through its **own browser interface**, rather than depending on the development panel.

- Introduced a minimal embedded web interface.
- Added camera controls and reception/playback of supported WAV audio.
- Established a clearer installer and development workflow.
- Recorded the firmware candidate identifier **`V1_03`**.

**Versioning note:** `V1_03` was a *technical firmware candidate identifier*. The history does not establish it as a separate public release named *JrBot V1.3*.

[Embedded portal work](https://github.com/JuniorNarciso26/JrRobot/commit/fc9f05a5a) · [Camera and WAV controls](https://github.com/JuniorNarciso26/JrRobot/commit/e2b824caf74ccdd9d1477b7004da84ce55afb046)

### September 13–14 — V1.4 and the refinements leading to V1.5

The next identifiable V1 milestone was **`JrBot_V1.4`**. Its commits improved camera behavior and the path for recording audio sent from a connected phone/computer.

The repository then recorded a succession of incremental firmware identifiers:

| Build | Documented evolution |
| --- | --- |
| **V1.4** | Camera and browser-to-robot audio improvements; firmware explicitly identified as V1.4 |
| **V1.4.1** | Fix for recording with the microphone without opening the camera |
| **V1.4.2** | Follow-up firmware identification and consistency corrections |
| **V1.4.3** | Expanded Wi-Fi/network diagnostics and related firmware identification |
| **V1.5** | New candidate identifier and preparation of the local HTTPS capability |

These are **recorded builds and candidates**; a commit naming a version is not, by itself, proof of a separate public release or full physical acceptance.

[V1.4 firmware identification](https://github.com/JuniorNarciso26/JrRobot/commit/d6cb5f7e2b2597c22a2c96d19fac618fac052966) · [V1.4.1 fix](https://github.com/JuniorNarciso26/JrRobot/commit/0b0fa36c13b91ff92517bd1d1f3699e827a557e) · [V1.5 candidate](https://github.com/JuniorNarciso26/JrRobot/commit/822848659357d67677f0637576cc2467c2559039)

### September 14 — V1.6 and V1.6.1: a usable local interface

This was the step that brought the earlier experiments together in a more complete browser-controlled JrBot.

- Added configurable camera resolutions, quality settings and continuous image handling.
- Integrated camera preview and **half-duplex push-to-talk (PTT) intercom** controls.
- Brought the internal portal and firmware identifiers into the V1.6 line.
- Consolidated **`JrBot_V1.6.1`** and standardized the reference camera orientation.

The V1.6.1 workstream was merged into `develop` on **September 14**. It became the starting point for the documented V1.6.2 changes.

[V1.6 camera and PTT integration](https://github.com/JuniorNarciso26/JrRobot/commit/cd8e42fc5b47d30595fa6369e648bd078aca63df) · [V1.6.1 merge](https://github.com/JuniorNarciso26/JrRobot/commit/33bed9aa3e7df465ce8a853bfc1e120f40a06cd9)

### September 19 — JrBot_V1.6.2: easier installation

The installer began discovering active remote branches dynamically, instead of relying on a fixed menu.

- Allowed selection of suitable `feature/*`, `fix/*`, and `hotfix/*` branches for testing.
- Hid `archive/*` branches from the normal selection workflow.
- Indicated the currently selected branch with `[ATUAL]`.
- Preserved the steps for updating, compiling, flashing and opening the development panel.

This release primarily improved the **development and installation experience**, rather than changing the robot's hardware or its existing PTT behavior.

### September 23 — JrBot_V1.6.3: real-time video and two-way audio

**Live WebRTC** changed the way people could interact with JrBot.

- Replaced the principal push-to-talk experience with **bidirectional live audio** using PCMA/G.711A at 8 kHz mono.
- Delivered OV5640 JPEG video through a WebRTC DataChannel/SCTP.
- Added local HTTPS signaling, independent audio-direction mute and video quality profiles.
- Validated simultaneous I2S reception/transmission on HW04, with BCLK and WS shared by microphone and amplifier.
- Improved PSRAM allocation to protect internal memory during ICE/DTLS/SCTP.

**Physical tests confirmed:** JrBot-to-phone and phone-to-JrBot sound, continuous video, and independent muting of both audio directions.

The earlier `JrBot_V1.6.2.1` camera-rotation hotfix was incorporated into this line, not promoted as a separate major product version. The repository also adopted the **MIT License** with documented third-party licensing boundaries.

**Limitations at this milestone:** acoustic echo cancellation was not implemented, WebRTC did not operate across the public Internet, and occasional video DataChannel backpressure remained observable.

### September 28 — JrBot_V1.7.04: a more reliable foundation

After adding live communication, the next priority was **coordination and reliability**.

- Removed repetitive automatic `/status` requests from the App.
- Introduced `jr_mode_manager` for explicit operating modes.
- Introduced `jr_resource_manager` to arbitrate I2S use between microphone, playback and Live.
- Added metrics for resource acquisition, release, busy events and ownership mismatches.
- Physically tested `IDLE → LIVE → IDLE` and the release of shared resources after a Live session.

**HW04 validation included:** recording, playback, full-duplex WebRTC, JPEG over DataChannel, I2S returning to owner `none`, `i2s_release_mismatch=0`, and no reboot or watchdog failure during the final recorded test.

**`JrBot_V1.7.04` became the last stable firmware version promoted to `main` in this history.** It provided the foundation for the next major integration.

Remaining improvements included AEC, video buffering, Internet-wide Live access and offline voice development in the separate V2 line.

## September–October 2026 — JrSkill Network adds a new dimension

### Colosseum 2026 — From fixed features to downloadable Skills

During the **Colosseum / Crypto World's Fair 2026 hackathon**, JrSkill Network was developed as an **upgrade to JrBot**, extending the robot beyond capabilities compiled directly into its firmware.

Its initial ecosystem connected:

1. **Skill creation and publishing:** structured, declarative JSON describing approved robot actions.
2. **JrSkill Store:** discovering Skills and acquiring licenses with a wallet.
3. **Solana Devnet:** Skill, Offer and License accounts supporting ownership and purchases in **test SOL**.
4. **My Skills:** discovering licensed Skills, selecting them and executing safe actions on the physical robot.

The work progressed from local JSON execution to an on-chain Skill PDA, retrieval and verification, wallet authentication, license purchases, and finally physical execution of a licensed Skill.

**October 7 — Physical execution validated:** the independently authored `jrteste` Skill ran on JrBot through the App and development panel. The reference firmware and panel were `JrBot_V1S_APP_03` and `JRBOT-PANEL-V1S-SKILLS-15`.

**October 10 — Initial milestone documented as complete:** the hackathon MVP had demonstrated the integrated Devnet acquisition, licensing and physical-execution journey. This is a **development milestone**, not a claim that a production Mainnet marketplace was launched.

The expanded creator/developer platform remains an ongoing area of development.

[Full Colosseum development diary — Kickoff and Day 1–9](../06-colosseum-history/HACKATHON_DEVLOG.md) · [Day 8 physical and transaction evidence](../06-colosseum-history/HACKATHON_DEVLOG/day8.md) · [Day 9 milestone summary](../06-colosseum-history/HACKATHON_DEVLOG/day9.md)

## October 26, 2026 — JrBot V1.8.1S: Wi-Fi setup and local name recovery

This update follows the V1.8S JrSkill integration on `main`. The firmware ID is
`JrBot_V1.8.1S`.

- Reduced the Wi-Fi/WebRTC memory footprint in the RAM-light development line.
- Added an open `JrBot-Setup-XXXXXX` access point and a phone setup page matching
  the JrBot panel. It opens after about 30 seconds without a startup Wi-Fi
  connection or on a three-second BOOT-button request.
- Added network scanning, saved-network management and connection testing. A
  candidate is saved only after authentication and an assigned IP; failed
  password and network attempts leave saved credentials unchanged.
- Kept `jrbot.local` as the mDNS name and added a retry after an mDNS
  initialization failure. Access still requires a connected local network
  that permits multicast DNS and, for HTTPS, valid local certificate material.
- Updated the `main` installer to use the RAM-light build configuration and a
  build directory isolated by the firmware version.

The firmware build and software checks are recorded in [Testing](../04-firmware-development/TESTING.md).
The owner later reported physical and memory tests OK for this version, without
new per-scenario logs or numerical memory measurements. The earlier HW04 and
Devnet records remain evidence for their own tested builds.

## November 7, 2026 — Experimental Live Audio Lab candidate

The `feature/live-audio-lab` branch identifies its firmware as
`JrBot_V1.8.1S_AUDIO_LAB_01`. It adds a **Mesa de Áudio** to the Live page on
`jrbot.local`, with separate speaker volume, microphone gain, echo ducking,
noise gate and 20/40/60 ms pre-buffer controls. The settings are temporary in
RAM. Diagnostics expose queue occupancy, underruns, microphone clipping and
RAM/PSRAM availability. This is a tuning tool, not full acoustic echo
cancellation.

The owner reported that communication works and sounds good with headphones.
Reducing microphone gain and speaker volume while the microphone was open
improved the perceived sound. The owner still hears their own voice return
with a delay, even while away from the robot. No exact gain settings, delay
measurement or RAM/PSRAM time series were supplied. The result is a positive
audio field report, not complete validation of memory stability, video and
I2S behavior. See the [Audio Lab guide](../04-firmware-development/LIVE_AUDIO_LAB.md)
for controls, diagnostics, limits and the remaining test procedure.

The PR #57 source compiled locally with ESP-IDF 5.5.5 and the RAM-light
profile. HW04 pin-policy checks and embedded JavaScript syntax passed. This
software validation did not flash hardware or measure Live RAM/PSRAM use.

## November 7, 2026 — JrBot V1.8.2S: Mesa Mix for Live audio

The tested Audio Lab work from [PR #57](https://github.com/JuniorNarciso26/JrRobot/pull/57) was promoted to `main` and given the final firmware identifier **`JrBot_V1.8.2S`**. The robot's Live page now presents **Mesa Mix · Live** for adjusting WebRTC speaker volume (0–100%), microphone gain (25–150%), echo ducking (0–100%), noise gate (0–2000 PCM) and pre-buffer (20/40/60 ms). The controls apply during the conversation, live in RAM and return to defaults after a reboot. The existing audio queue is reused, and the page exposes underruns, clipping and RAM/PSRAM diagnostics.

The owner reported good communication with headphones and better perceived sound after reducing microphone gain and speaker volume. Their own voice still returns with a delay, even away from the robot. The exact V1.8.2S source passed an ESP-IDF 5.5.5 RAM-light build, HW04 pin-policy checks and embedded JavaScript syntax validation. No exact tuning values or RAM/PSRAM time series were supplied for V1.8.2S; this release does not claim complete acoustic echo cancellation or measured long-session memory stability. See the [Mesa Mix guide](../04-firmware-development/LIVE_AUDIO_LAB.md).

## November 7, 2026 — JrBot V1.8.3S: Wi-Fi management after connection

The connected-mode Wi-Fi dashboard from [PR #59](https://github.com/JuniorNarciso26/JrRobot/pull/59) was integrated into `main`. The firmware version is **`JrBot_V1.8.3S`**, preserving the V1.8.2S Live audio Mesa Mix.

- Added a third tab, **Wi-Fi**, beside **Control** and **Memory**, in the local browser portal `https://jrbot.local`.
- Exposed connected/disconnected status, current SSID, assigned IP, RSSI in dBm, Wi-Fi channel and operating mode without returning stored passwords. RSSI refresh is manual or optional every 10 seconds while the tab is visible.
- Added an on-demand scan (up to eight displayed networks), explicitly blocked during Live/autonomous operation to avoid disrupting audio/video.
- Allowed saving and updating Wi-Fi credentials, viewing up to **four profiles**, and selecting one for the next reboot, with existing NVS storage.
- Added **Excluir / Delete** next to each stored network, with confirmation. Deleting the selected network warns that initial provisioning may be needed on the next boot.
- Kept the current connection running while saving or selecting a profile. Switching networks requires a **separately confirmed reboot**, which can change the device IP.
- Retained the first-run phone provisioning and USB/Serial recovery paths.
- Added HTTPS-only `/wifi-manager` endpoints for status/profiles/scan and profile writes. The general `/cmd` endpoint continues to block network Wi-Fi configuration commands.
- Avoided a persistent ESP32 scan task, using the existing scanner and temporary PSRAM-backed response buffers.

**Acceptance and limits:** the owner approved the dashboard for promotion. Source integration and the V1.8.3S identifier were confirmed in GitHub. Full independent hardware/network-switch regression, compile-size and RAM/PSRAM measurements were not documented as completed in this integration. HTTPS origin restrictions do not replace login/authentication, and the dashboard must not be exposed to the public Internet. See [Wi-Fi Dashboard](../04-firmware-development/WIFI_DASHBOARD.md) and [Project Status](PROJECT_STATUS.md).

## Where this history stands

The current source version on `main` is **JrBot V2.0**; prior V1.8 milestones are preserved below. V1.8.1S remains the last separately owner-confirmed full physical-and-memory baseline; V1.8.2S added Mesa Mix with a positive headphone audio report and delayed self-voice return still known; V1.8.3S adds the connected Wi-Fi management dashboard, network profile deletion, and confirmed network switching.

This Changelog preserves **the path already taken**. See the [Roadmap](ROADMAP.md) for the next steps and [Project Status](PROJECT_STATUS.md) for today's verified capabilities.


## November 12, 2026 — V1.8.4S ROAM_01 (develop, hardware validation pending)

ESP32-S3 Wi-Fi station now uses the ESP-IDF built-in all-channel scan sorted
by RSSI on each connection, instead of fast-first-match scanning. A native
-70 dBm low-RSSI event (not native automatic roaming) triggers a bounded,
asynchronous, same-SSID AP scan **in idle mode only**; a different BSSID must
offer at least 12 dB better signal before a controlled reconnect. Scans
have at least 90 seconds of cooldown; no polling task or periodic scan.
No forced roaming during Live. `JR_WIFI_ROAM` logs decisions.
The 2.4 GHz-only ESP32-S3 does not support 5 GHz regardless of SSID naming.
Changing BSSID briefly interrupts existing network sessions; TLS errors
with good RSSI remain a separate issue. Requires ESP-IDF build and HW04
hardware / Live test before promotion to main.
Build stamp: `2026-11-12_JrBot_V1.8.4S_ROAM_01`.

## November 12, 2026 — V1.8.4S DIAG_01: intermittent Wi-Fi / HTTPS investigation (develop)

Diagnostic-only follow-up. The owner reported losing Wi-Fi/panel access some
time after connecting; earlier HW04 logs showed HTTP URI handler failures,
TLS handshake errors and subsequent Serial monitor interruption. Added
Wi-Fi association/disconnection/IP event logs, retry/exhaustion messages,
a lightweight 30-second connected SSID/BSSID/RSSI/IP and internal/PSRAM heap
snapshot, and request-route/error/heap logging on failed HTTP handlers.
Queries and passwords are not logged. TLS handshake errors occur before
route dispatch and remain printed by ESP-IDF; correlate to periodic samples.
Build stamp: `2026-11-12_JrBot_V1.8.4S_DIAG_01`.
No recovery/connection behavior changed; hardware/ESP-IDF acceptance pending.

## November 12, 2026 — V1.8.4S follow-up: automatic recovery reboot

After a HW04 log showed startup falling into captive setup at ~30 seconds,
followed by successful saved-profile failover only after a manual restart,
`develop` received an automatic, **one-time software reboot** on initial
startup connection timeout. A paired RTC marker blocks reboot loops;
a successful STA IP or cold boot clears the marker. If the second boot
still cannot connect, captive setup opens as before. This is a follow-up
fix to the V1.8.4S candidate, not a new promoted production version.
Host-unit test added; ESP-IDF compilation and physical acceptance remain pending.

## November 12, 2026 — V1.8.4S candidate (develop only)

- Synchronized `develop` with the source tree of approved V1.8.3S `main` while retaining both Git histories.
- Added bounded retries and automatic fallback to other NVS-saved Wi-Fi profiles when the selected network does not connect.
- The first alternative that obtains an IP is persisted as the preferred profile for the next reboot; failed candidates are not persisted.
- Kept the existing four-profile limit, captive portal recovery, manual Wi-Fi panel and no extra polling task.
- Added a mock NVS/driver regression scenario. Real ESP-IDF build and HW04/Live/memory regression are pending. No merge to `main`.
