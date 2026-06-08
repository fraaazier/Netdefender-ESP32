# 🛡️ Netdefender-ESP32: Comprehensive Architecture Overview
# RELEASE IS STILL RAW, ERRORS POSSIBLE!!!
## 📝 Project Concept (What & Why)

### What is Netdefender-ESP32?
**Netdefender-ESP32** is an autonomous, ultra-low-overhead hardware-software ecosystem tailored for wireless airspace auditing, Layer 2 radio reconnaissance, and proactive wireless intrusion detection (WIDS) within the 2.4GHz RF spectrum. 

Unlike standard network tools that operate within the upper layers of the OSI model (requiring full IP assignment and cryptographic authentication), this system repurposes a standard dual-core ESP32 microcontroller into an independent **Promiscuous Mode Frame Sniffer**. By capturing raw IEEE 802.11 management, control, and data frames directly from the air interface, the node executes passive security monitoring without leaving any digital footprint or connecting to target access points.

### Why is this Architecture Necessary?
Modern enterprise and residential Wi-Fi infrastructures are inherently susceptible to physical air-interface exploits. Malicious actors leverage cheap hardware to spoof management frames, disrupt critical communications, or deploy phishing access points. Netdefender-ESP32 solves this vulnerability gap by acting as a persistent, low-cost defensive sentry node. It monitors localized airspace anomalies, records digital forensic evidence, and relays critical status updates in real-time.

---

## ⚡ Technical Deep Dive: Feature & Module Breakdown

The firmware implements a fully decoupled, state-driven modular architecture. Each subsystem runs independently inside non-blocking asynchronous timers governed by global control flags, allowing seamless operational shifting on the fly.

### 1. 🛡️ Layer 2 IDS (Intrusion Detection System) Engine
* **Passive Radio Interception:** Forces the ESP32 Wi-Fi baseband controller into a low-level promiscuous mode, bypassing hardware MAC address filtering to ingest every frame matching the 2.4GHz physical layer configuration.
* **Deauthentication Storm Analysis:** Implements real-time signature matching on Subtype fields. It explicitly monitors for `0x00C0` (Deauthentication) and `0x00A0` (Disassociation) frame floods used by attackers to disconnect clients for handshake harvesting or denial-of-service (DoS).
* **Rogue AP & Evil Twin Detection:** Continuously parses incoming beacon frames to correlate SSID strings with their physical BSSID (MAC) origins and RSSI baselines, flagging unauthorized network clones attempting credential phishing.
* **Execution Loop:** Runs on an isolated async routine every `6000ms` to update attack counters and clear stale environment buffers without blocking the network stack.

### 2. 📊 RF Spectrum & Channel Hopping Monitor
* **Automated Sequential Hopping:** Directs the hardware radio frequency synthesizer to rapidly cycle through available 2.4GHz channels (Channels 1 to 13) to capture a complete statistical representation of the airspace.
* **RSSI Telemetry Mapping:** Measures the raw Received Signal Strength Indication (RSSI) of all surrounding ambient transmissions to compute localized noise floors and identify suspicious high-output signal spikes.
* **Congestion Metrics:** Logs active device counts and frame density per channel, pointing out exactly which frequencies are compromised, highly congested, or used for covert data exfiltration.
* **Execution Loop:** Evaluates channel telemetry grids and pushes structured signal maps to system interfaces every `7000ms`.

### 3. 🔋 Hardware Power Management & Battery Saving
* **Dynamic Frequency Scaling (DFS):** Integrates directly with the native `esp_pm` framework. The chip dynamically scales down the Xtensa dual-core processor from `240MHz` to `80MHz` during idle processing states, reducing current consumption by up to 60%.
* **Modem Sleep Optimization:** Toggles automated radio power management via `WiFi.setSleep(true)`. The radio module drops into ultra-low-current states between standard 802.11 DTIM beacon delivery windows.
* **Light Sleep Interleaving:** Configures the hardware scheduler to seamlessly place the core components into light sleep during code execution gaps, optimizing the node for extended deployments on 18650 Li-Ion batteries or power banks.

### 4. 💾 Forensic Live PCAP Logger
* **SPI Bus Stream Management:** Commands an unbuffered hardware SPI pipeline tied directly to the local MicroSD card storage media interface (utilizing GPIOs 18, 19, 23, and 5).
* **Wireshark Compatible Framing:** Encapsulates captured raw binary 802.11 frame arrays into global standard PCAP file format headers on the fly.
* **Incident Forensics:** Retains precise cryptographic proof of network intrusion attempts for post-incident timeline evaluation. The internal data cache is fully flushed to the storage block every `10000ms` to preserve memory pool stability.

---

## 🌐 Integrated Operational Control Interfaces

To guarantee true infrastructure flexibility, the modular core interacts simultaneously with three completely isolated command and data interfaces:

### 📱 Local Web UI Switchboard
* Spawns a lightweight native HTTP daemon via the `WebServer` library bound to port `80`.
* Operates an independent, protected captive Wi-Fi Access Point (`Netdefender-Node`) with a WPA2 authentication layer.
* Serves a minimalist, dark-themed monospace HTML/CSS control switchboard optimized for ultra-fast rendering on mobile web browsers.
* Employs isolated GET routing endpoints (`/toggle?set=...`) for deterministic state transitions.

### 💬 Secure Telegram Bot Agent
* Leverages the `UniversalTelegramBot` wrapper executed over a secure TLS WebSocket connection handled by `WiFiClientSecure`.
* Extends execution efficiency by enforcing explicit SSL chain bypass (`tgClient.setInsecure()`), preventing handshake calculation bottlenecks.
* **Anti-Hijack Security:** Runs a strict string-validation routine matching the incoming JSON `chat_id` token against the user's hardcoded ID to silent-drop unauthorized third-party inputs.
* Evaluates external query queues for remote commands (`/status`, `/ids_on`, `/ids_off`, etc.) every `2500ms`.

### 💻 Local Serial CLI Interpreter
* Establishes a direct, unmanaged local hardware UART serial pipeline communicating at a stable `115200` baud rate.
* Executes proactive line cleaning (`input.trim()`) to prevent instruction parsing failures caused by non-standard terminal carriage returns (`\r\n`).
* Provides bare-metal low-level developers with immediate runtime status logs and manual hardware feature overrides.
