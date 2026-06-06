
# 🔐 CredSniper v4.0

CredSniper is an advanced **ESP8266 Wi‑Fi security assessment tool** designed for education, learning, and authorised security testing.

![image](https://iili.io/fZQSlIt.md.png)

---
[![Wiki](https://img.shields.io/badge/Wiki-Documentation-blue)](https://github.com/DawoodKhan5218/CredSniper/wiki)

## 📖 What is CredSniper?

CredSniper is a standalone project that runs entirely on an **ESP8266 NodeMCU**.  
It creates a Wi‑Fi access point with a **web‑based control panel** where different Wi‑Fi attack simulations can be tested in a controlled and authorized environment.

This tool focuses on **education, learning, and security awareness**.

---

## ✨ Features

### 🎭 Evil Twin Attacks
Create fake Wi‑Fi access points with realistic login pages to demonstrate how users can be tricked into entering credentials.  
**6 built‑in social‑media templates** are included: **Google (3‑step verification), Instagram, Facebook, TikTok, Pinterest, and Apple**.  
The Google template displays the victim’s email address on the password page.

### 💣 Deauthentication Attacks
Disconnect devices from a Wi‑Fi network (single‑target broadcast or **Deauth‑All** for multiple networks).  
Deauth automatically pauses during **Router Rescue password verification** to prevent self‑deauth.

### 📡 Beacon Spam
Create 70+ fake Wi‑Fi networks (One Direction song titles) across channels 1, 6, and 11 using a proven Spacehuhn‑style method.  
The admin panel shows a live packet counter.

### 🔧 Router Rescue Mode (PhiSiFi‑style)
Serves a professional **“Firmware Update Failed”** page that asks for the Wi‑Fi password.  
The password is **verified against the real network**:
- ✅ Correct → saved as valid, attack ends  
- ❌ Wrong → attack stops, victim sees an error page  
A 20‑second progress bar is shown during verification.  
Works seamlessly with iPhone and Android captive portals.

### 🕵️ Stealth Mode
Toggle the admin AP’s SSID between **visible** and **hidden** from the control panel.  
The setting is saved to SPIFFS and persists after reboot or after stopping attacks.

### 🎲 Rogue AP (Custom SSID)
Create an open fake access point with any SSID and channel you choose – no existing network required.

### 🔴 Live Credential Logs
A dedicated web page (`/logs`) displays captured credentials **in real time** (auto‑refresh every 2 seconds).  
Each entry shows the timestamp, captured user/password, and the template used.  
Includes a **Clear Logs** button (clears only the live buffer, not the SPIFFS file) and a **🛑 STOP ATTACK** button that returns to the admin AP.

### 🛑 Manual Attack Stop
The evil twin **remains active** after a victim submits credentials, allowing multiple captures until you manually stop the attack via the Live Logs page.

### 🖥️ Web Interface
Full control through a clean, dark‑themed browser dashboard at `http://192.168.4.1`.  
Real‑time status displays the current mode, selected network, template, stored credentials, storage/RAM usage, and uptime.  
One‑click buttons for scanning, network selection, template switching, and launching attacks.

### 🔑 Credential Capture
All form fields from phishing pages are automatically saved to SPIFFS (`/credentials.txt`).  
A **View Credentials** page shows the total number of entries and the full file content.  
You can **download** the file or **delete all** credentials with a single click.

### 📶 Network Scanning
Scan for nearby Wi‑Fi networks and display them in a table with **signal strength percentages**, BSSID, and channel.  
Networks can be selected/deselected directly from the table.

### 🧩 Custom HTML Upload
Upload your own `.html` templates through the admin panel.  
Uploaded templates appear in a list and can be set as the current phishing page or deleted.

### 🛡️ Stable & Optimised
The admin panel is sent as a **single large String** – no more chunked‑streaming timeouts.  
A heap‑health watchdog automatically reboots the device if memory runs critically low.

---

## 📦 Included Templates

| Template | Description |
|----------|-------------|
| 🔵 Google (3‑Step) | Realistic Google login with email → password → 2FA flow |
| 📸 Instagram | Instagram login page with session expired notice |
| 👥 Facebook | Facebook login with security alert |
| 🎵 TikTok | TikTok login with session timeout warning |
| 📌 Pinterest | Pinterest login with authentication required notice |
| 🍎 Apple | Apple ID sign‑in with verification prompt |
| ⚠️ Firmware Update | Professional router‑recovery page (for Router Rescue) |

---

## 🔌 Hardware Requirements

- ESP8266 NodeMCU (ESP‑12E / ESP‑12F)  
- Micro‑USB cable  
- Optional: External Wi‑Fi antenna (for lab testing)

---

## 💻 Software Requirements

- Arduino IDE  
- ESP8266 Board Package (≥2.5.0)  
- USB driver (CH340 / CP210x)

---

## 🌐 Supported Devices

- Android smartphones  
- iOS devices (iPhone, iPad)  
- Windows  
- Linux  
- macOS

---

## ⚙️ Installation

### 📥 Download CredSniper.ino file

💾 Upload the Code

1. Open **CredSniper.ino** in Arduino IDE  
2. Select **NodeMCU 1.0 (ESP‑12E Module)**  
3. Set **Flash Size** to **4MB**  
4. Click **Upload**

---

## 🌐 Device Access

After flashing the firmware:

| Item | Value |
|------|-------|
| **Wi‑Fi SSID** | `CredSniper` |
| **Password** | `dewdew5218` |
| **Admin Panel** | `http://192.168.4.1` |
| **Live Logs** | `http://192.168.4.1/logs` |

---

## 🎯 Use Cases

1. Wi‑Fi security learning  
2. Cybersecurity education  
3. Authorised penetration testing  
4. Security awareness demonstrations  
5. ESP8266 experimentation

---

## 🙏 Credits
**Spacehuhn Deauther** – inspiration for beacon spam & deauth logic  
**Open‑source community** – various phishing template designs  

---

## ⚠️ Disclaimer

**CredSniper is intended for educational and authorised security testing only.**

- Use only on networks you own  
- Or networks you have explicit permission to test  
- The developer is not responsible for misuse, damage, or illegal activity

---

# 📜 License
This project is released for educational purposes.  
See the LICENSE file for more information.
