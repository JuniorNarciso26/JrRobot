# JrBot Wi-Fi provisioning — integration candidate

Issue: [#55](https://github.com/JuniorNarciso26/JrRobot/issues/55)  
Candidate branch: **develop**  
Build ID: **JrBot_V1S_WIFI_02**  
Evidence: integrated implementation and host/static checks; **ESP-IDF build and full-robot physical validation pending**.

## User flow

- Existing saved profiles reconnect automatically using DHCP. Legacy JrBot SSID/password are imported if no profile exists, once only.
- Profiles saved during the isolated experiment retain the same NVS namespace/format. The nvs partition offset/size is unchanged. Do not erase flash to install this candidate.
- With no usable profiles, connect the phone to **JrBot-Config**. The HTTP portal on 192.168.4.1 redirects to the setup server on port 8080.
- Choose a network, enter password, connect. Failed connection keeps setup available; successful connection/profile save ends setup automatically.
- Reconnect the phone to the home network and open **http://jrbot.local/** for the normal JrBot portal.
- Use **http://jrbot.local/wifi** and its Configurar Wi-Fi button to start a new setup session, then join JrBot-Config.
- Serial fallback: **wifi_prov_start**. Existing wifi_config_pct/wifi_clear persistence/restart flow is retained, using an import-pending marker.
- Saved-profile Habilitar/Desabilitar controls candidate eligibility; it does not force a connection switch. Maximum eight profiles.
- This candidate uses DHCP as requested. Old static IPv4 settings are not applied; new serial static requests are rejected with an explicit DHCP message.
- The BOOT button is not assigned a new role in this integration.

## Architecture and compatibility

Four local Espressif components are copied from the validated experiment: esp_wifi_service, esp_config_manager, esp_service and adf_event_hub. The Wi-Fi service is the sole driver/selector owner; the old direct esp_wifi_connect loop is removed. The legacy jr_wifi status and command APIs remain available.

The provisioning HTTP server uses **8080 / control 8081**. The existing robot HTTP server remains on **80 / control 32768**, and HTTPS on **443 / control 32769**. Root/captive requests redirect only while provisioning is active. Stopping provisioning does not stop the normal robot portal. The main portal adds an explicit POST setup action with the same required X-JrBot-Command header policy as its command endpoint.

mDNS 1.14.0 advertises **jrbot.local**, tracks interface/IP events, and never persists a fixed IP. DHCP can return the same lease or a different one. Computer/phone must share the robot's LAN and the network must permit multicast discovery.

The experiment's profile encryption format and public demo key are retained to avoid losing profiles. This is not hardware-backed NVS encryption and is not a claim that the published key protects secrets against extraction. A separate storage-security review can replace this format with migration later.

The existing local HTTPS certificate/key requirement remains. mDNS does not make a certificate valid for jrbot.local; test TLS trust/name compatibility separately. Camera, OLED pin map, audio framing, Live and JrSkill runtime implementation are unchanged.

## Install and test

From the repository root:

```bat
INSTALAR.bat build
```

Select **develop**. It uses **build-v1s-wifi-develop** and **sdkconfig.v1s-wifi-develop**, separated from older builds. Installer enforces WebSocket and DTLS-SRTP options in existing configs. Then run **INSTALAR.bat flash** selecting develop. Do not run erase-flash.

1. Confirm BOOT/build ID JrBot_V1S_WIFI_02, no unexpected reboot.
2. Existing experiment or legacy profile reconnects; ping jrbot.local resolves to current STA_GOT_IP.
3. Open http://jrbot.local and confirm the normal robot interface.
4. Open /wifi and start setup; join JrBot-Config and confirm automatic/manual page on 192.168.4.1.
5. Wrong password stays retryable; correct password saves, closes setup, normal portal returns.
6. Check saved-profile deletion/toggle and that deleted legacy profiles are not reimported after reboot.
7. Restart robot and router; name/reconnection behavior.
8. OLED face, camera /capture, microphone recording, speaker playback.
9. HTTPS/Live camera and both audio directions using locally configured certificate material.
10. JrSkill authenticated license discovery and physical execution.

Known deferred checks from the experiment: intermittent captive-portal recovery and resolving the hostname after an actual DHCP lease change. These are not claimed fixed by this integration. Full-robot resource/concurrency behavior needs this new bench run.

**Do not promote develop to main until these candidate checks are reviewed.**


## Checks performed for this candidate

- Fixed HW04 pin-map generation and two host C11 pin-policy cases passed.
- Wi-Fi integration compiled with a mocked backend under host GCC/UBSan; eight migration/status cases passed: fresh, legacy, experiment, deleted, pending configure, pending clear, serial save/restart and serial clear/restart.
- Run `python tests/wifi_integration/run.py` to reproduce these mocked checks.
- Static review checked dependency paths, single driver ownership, migration markers, ports/routes, installer labels and version metadata.
- No full ESP-IDF build, Windows installer execution or physical test of this integrated candidate has been performed here.


## Bench failure and system-event stack correction

The first integrated candidate compiled/flashed on ESP-IDF 5.5.5 and booted with HTTP/HTTPS and a saved profile. User log at 12:28 on 12 October shows **stack overflow in sys_evt** during selection/connection, followed by reboot. This invalidates physical acceptance of WIFI_01; the exact overflowing call cannot be decoded from this log alone without the matching ELF.

The standalone working example specified CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE=4096; the integration omitted that requirement. WIFI_02 sets 4096 in defaults and the installer updates existing sdkconfig files with the same value before build. The build directory remains isolated from older releases. Confirm WIFI_02 in the next boot log, successful STA_GOT_IP, stable uptime and access to jrbot.local before continuing regression tests. No NVS erase is required. Compilation/physical validation of WIFI_02 pending.


## WIFI_06 — isolated mDNS memory trial (13/10/2026)

Compared main `6638a4c` with develop `4896652` after the owner reported working
Live on main. The camera implementation and main initialization are byte-identical.
The Live peer/mic/play stacks are unchanged (25/8/12 KiB); the Live diff only adds
memory snapshots and failure stages. The integration adds Wi-Fi service/profile
storage and mDNS, changes the socket limit from 16 to 32, expands HTTP/HTTPS route
capacity from 16/20 to 32/40, and explicitly sets the system-event stack to 4096.
The installer also selects a separate local sdkconfig for Wi-Fi builds; untracked
settings in the owner's old sdkconfig have not been inspected.

Physical logs show two failures: after successful WebRTC and DataChannel setup,
the camera cannot allocate 16384 bytes of DMA memory (largest block 13312);
after an earlier photo, the play task cannot allocate its 12288-byte stack
(largest internal block 7680). Both are consistent with pressure/fragmentation in
internal RAM, not exhaustion of the 8 MiB PSRAM or the application Flash partition.

A concrete additional allocation source is mDNS 1.14.0, whose default heap
allocator explicitly requests INTERNAL|8BIT capabilities, independent of the
application's generic malloc-to-PSRAM setting. Source:
https://github.com/espressif/esp-protocols/blob/mdns-v1.14.0/components/mdns/mdns_mem_caps.c
Configuration:
https://github.com/espressif/esp-protocols/blob/mdns-v1.14.0/components/mdns/Kconfig

This trial changes only mDNS heap allocation to SPIRAM. Its task stack, the Live
stacks, camera initialization order, socket limit, and route capacities stay as in
WIFI_05. The installer patches the existing Wi-Fi sdkconfig choice too, because
sdkconfig.defaults alone does not override an already generated config.

New startup logs: JR_WIFI_MEMORY before/after profile/provisioning/service/mDNS;
MEMORY_CONFIG reports effective mDNS allocation, event/mDNS stack sizes, socket
limit; MDNS_MEMORY reports before/after initialization. Live logs include
internal DMA free bytes and largest block, also forwarded to the browser log.
These measurements include concurrent task activity and are not exclusive
per-component allocation accounting.

Test: install develop WIFI_06, connect Serial before resetting the board to
capture startup, open one HTTPS tab, start VGA Live without taking a photo first,
stop Live, capture a photo, start Live again. Collect both Serial and Live logs.
Check allocations=PSRAM, successful audio/video, and absence of DMA/task errors.
No hardware result is claimed yet. A failed trial will justify isolating the next
integration difference (e.g. socket limit), then the Wi-Fi service backend if needed.
Do not roll back sys_evt to its old default: it overflowed during this integration.
