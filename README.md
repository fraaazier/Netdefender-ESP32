# NetDefender-ESP32 (2FA & IDS Spectrum Monitor)
# IS AN UNFINISHED VERSION! Because of this, some functions may not work correctly.
An advanced, low-level hardware solution for wireless auditing and network state monitoring. This firmware transforms an ordinary ESP32 microcontroller into an autonomic Layer 2 Wi-Fi Intrusion Detection System (IDS) and secure network node protected by Two-Factor Authentication (2FA).

---

## 🔒 Core Capabilities

### 📡 Real-Time IDS Spectrum Monitor
The firmware bypasses standard network layer filters by enabling **802.11 Promiscuous Mode**. The core engine directly decodes incoming wireless frames on the physical layer to detect ongoing infrastructure attacks:
* **RF Jamming Detection:** Continuously sniffs raw management frames (`0xC0` and `0xA0`) to catch malicious deauthentication or disassociation flood attacks in progress.
* **DDoS Storm Warnings:** Calculates total frame density per 3-second window. Triggers critical logs in the serial terminal if packet saturation limits are breached.

### 🛡️ Secure 2FA Cryptographic Gateway
To prevent unauthorized configuration or log manipulation, the node's local web matrix is locked behind dual-layer authorization:
1. **Master Password:** Strict string validation on the server side.
2. **Time-Based One-Time Password (TOTP):** Cryptographic 6-digit tokens generated through hardware-level `mbedtls` HMAC-SHA1 hashing of 30-second UNIX time steps. Fully compatible with **Google Authenticator**, **2FAS**, and **Yandex Key** apps without relying on external SMS services.

### 📋 MAC Whitelist Log Exceptions
Includes an embedded static matrix (`_ex_mac`) to register safe operational hardware identifiers. Packets arriving from whitelisted devices (e.g., your primary router or admin laptop) are ignored by the IDS processor to completely eliminate false-positive data spikes during heavy legal data transfers.

---

## 📊 Technical Specifications


| Parameter | Specification |
| :--- | :--- |
| **Architecture** | Tensilica Xtensa Dual-Core (ESP32) |
| **Frequency Spectrum** | 2.4 GHz ONLY (IEEE 802.11 b/g/n) |
| **Monitored Frames** | Layer 2 Management & Data Frames |
| **Crypto Stack** | Internal `mbedtls` SHA1 engine |
| **Web Gateway Port** | Default TCP Port 80 |

*Note: Due to architectural hardware limitations of the ESP32 chip, 5 GHz wireless spectrum streams cannot be monitored.*

---

## 🛠️ Configuration & Deployment

To build and compile the firmware, fill in the placeholder matrices inside the source file using standard alphanumeric lowercase formatting:

```cpp
// enter your home wi-fi router network name here
const char* _st = "your_wifi_name_here";        

// enter your home wi-fi network password here
const char* _ps = "your_wifi_password_here";  

// create a master password to unlock the admin web page
const char* _mk = "admin_secure_key";     

// enter the exact 20-character key from google authenticator app here
const char* _sd = "SECURESEEDXTRATWOREW"; 

// add trusted mac addresses here to exclude them from attack detection
const uint8_t _ex[_SZ] = {
  {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},   
  {0x11, 0x22, 0x33, 0x44, 0x55, 0x66}    
};
```

---

## 🧪 Testing the 2FA Logic

1. Scan the Base32 token `SECURESEEDXTRATWOREW` into your phone's authenticator app.
2. Flash the module, boot it, and find the node's local IP address in the serial monitor.
3. Open any browser connected to the same network and navigate to the node's IP.
4. Input your master password and the running 6-digit 2FA token to unlock the system core matrix.
