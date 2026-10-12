# ESP Wi-Fi Service — isolated JrBot experiment

Tracking: [Issue #55](https://github.com/JuniorNarciso26/JrRobot/issues/55)  
Branch: `test/esp-wifi-provisioning`  
Status: source copied; build and physical validation pending.

## Source and scope

Official source: [espressif/esp-adf](https://github.com/espressif/esp-adf/tree/4cdbcd2a1bfd5823220de5b76921df26d95c8a7d/components/esp_wifi_service/examples/wifi_service_example).  
Pinned upstream commit: `4cdbcd2a1bfd5823220de5b76921df26d95c8a7d`. The copied Wi-Fi service manifest identifies version **0.5.3**.

This directory contains the official standalone example and its five local ADF dependencies: esp_wifi_service, esp_config_manager, esp_cli_service, esp_service and adf_event_hub. Component C/HTML code is unchanged. The ESP-IDF component manager still downloads cJSON; an Internet connection is required for the first build.

Adaptations:
- Example dependency override paths point to the copied sibling `components/` directory.
- Defaults select HTTP provisioning with the embedded Web UI, and disable Bluetooth and MCP for this Wi-Fi-only experiment.
- Added this guide, source inventory and local ignore rules.

The JrBot firmware remains unchanged. On this experimental branch, the root `INSTALAR.bat` now routes directly to the standalone Wi-Fi example, with its own `build-wifi-only` directory and SDK configuration. It does not compile `firmware/` or open the JrBot Python panel.

## Windows installer

From the repository root run `INSTALAR.bat build` and select `test/esp-wifi-provisioning`. Once compilation succeeds, run `INSTALAR.bat flash` and select the same branch. The installer loads ESP-IDF and discovers the serial port using the existing helper. Use `INSTALAR.bat monitor` for logs. The default action builds and flashes the example.

A previous attempt through the root installer selected the legacy `sdkconfig.runtime-api-v1-02` and compiled `esp_peer` under `firmware/`, producing missing DTLS/SRTP symbols. Those paths identify the robot application rather than this Wi-Fi experiment. The routing fix prevents that unintended compilation; it does not claim to repair a local robot SDK configuration.

## Compile first

In an ESP-IDF **5.5.x** terminal, from an existing clean local checkout:

```text
git fetch origin
git switch test/esp-wifi-provisioning
cd experiments/esp-wifi-service/wifi_service_example
idf.py set-target esp32s3
idf.py build
```

If the local branch is not present yet, `git switch --track origin/test/esp-wifi-provisioning` creates it locally. Do not discard local changes to switch branches.

Report any build error in Issue #55 before changing the example. No build has been executed in the preparation environment: ESP-IDF, CMake and an ESP32 device are unavailable there.

## Flash on the bench

This is a complete standalone application. Flashing it temporarily replaces the JrBot application: robot face/audio/camera/Skills are not included. Have the known-good JrBot firmware and its local configuration available for restoration. A spare ESP32-S3 is preferable for the first trial.

The upstream partition layout is copied unchanged. The upstream `app_main.c` can erase NVS if initialization reports `ESP_ERR_NVS_NO_FREE_PAGES` or `ESP_ERR_NVS_NEW_VERSION_FOUND`. Therefore do not assume saved robot settings survive this experiment. **Do not run `erase-flash` as part of normal testing.**

Discover the actual serial port on your computer, replace `<PORT>`, then run:

```text
idf.py -p <PORT> flash monitor
```

USB is used for flashing and observing logs. Wi-Fi credentials must be entered through the phone, not through the console.

## Phone test

1. Power on with no saved **experiment** profiles. The experiment uses the namespace `wifi_svc_ex`; existing JrBot settings are not its profiles.
2. In the phone Wi-Fi settings connect to **ESP_SVC_XXXXXX**, the upstream default SSID. The prefix is intentionally unchanged for the initial trial.
3. If the phone reports no Internet, keep connected to the device network.
4. Open the captive portal. If it does not open automatically, use the gateway/router address shown for that Wi-Fi network in the phone settings.
5. Select or type a 2.4 GHz network SSID. First submit an incorrect password and observe the reported failure and ability to retry.
6. Submit the correct password. Confirm connection and **STA_GOT_IP** in the logs.
7. Power off/on. Confirm reconnection without entering credentials again.
8. Turn the router off/on and record recovery. Record behavior when the saved network is unavailable.
9. Test changing to a different network. If a saved profile prevents the portal from starting automatically, the upstream diagnostic console provides `wifi prov start`; record this dependency as a gap for the eventual phone-only product flow.
10. Record results in Issue #55. Remove passwords and sensitive information from evidence.

The eventual JrBot SSID, AP password, physical recovery button and credentials migration will be decided during integration, after the standalone experiment passes.

## Evidence to record

- Exact experiment commit and ESP-IDF version.
- Build result and flash result separately.
- Phone model, OS and browser.
- AP discovery; automatic/manual portal; wrong password; valid password/IP.
- Power cycle, router restart, unavailable network and network change.
- Logs/screenshots without passwords.
- Pending failures and recovery/restore result.

A successful build is not a physical pass. After the standalone pass, update Issue #55 and integrate into a firmware candidate; test HTTP/HTTPS, OLED, camera, audio, WebRTC and JrSkill before promotion.

## License

All copied third-party files retain their original notices. See the LICENSE in each component directory. The example uses Espressif's modified MIT license, whose text is also available in `components/esp_wifi_service/LICENSE`. The repository's original MIT license does not replace those terms.
