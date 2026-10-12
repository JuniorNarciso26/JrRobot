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
