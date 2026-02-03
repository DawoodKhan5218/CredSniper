---

# 🔐 CredSniper v3.0

CredSniper is a beginner‑friendly **ESP8266 Wi‑Fi security assessment tool** designed to help users understand wireless attacks and improve Wi‑Fi security awareness.

![image](https://iili.io/fZQSlIt.md.png)

---

## 📖 What is CredSniper?

CredSniper is a standalone project that runs entirely on an **ESP8266 NodeMCU**.  
It creates a Wi‑Fi access point with a **web‑based control panel** where different Wi‑Fi attack simulations can be tested in a controlled and authorized environment.

This tool focuses on **education, learning, and security awareness**.

---

## ✨ Features

### 🎭 Evil Twin Attacks
Create fake Wi‑Fi access points with realistic login pages to demonstrate how users can be tricked into entering credentials.

---

### 💣 Deauthentication Attacks
Disconnect devices from a Wi‑Fi network to study reconnection behavior and wireless security weaknesses.

---

### 📡 Beacon Spam
Create multiple fake Wi‑Fi networks (SSIDs) to simulate crowded wireless environments and RF stress testing.

---

### 🖥️ Web Interface
Control all features through a clean and simple browser‑based dashboard without using serial commands.

---

### 🔑 Credential Capture
Save submitted login attempts locally on the device for educational analysis and security awareness training.

---

### 📶 Network Scanning
Scan and display nearby Wi‑Fi networks to understand signal strength, channels, and network visibility.

---

### 🧩 Custom HTML Upload
Upload your own custom HTML pages for captive portals, allowing full customization of fake login designs.

---

### 🎨 6 Default Phishing Templates (Google, Instagram, Facebook, TikTok, Pinterest, Apple)

---

## 🔌 Hardware Requirements

- ESP8266 NodeMCU (ESP‑12E / ESP‑12F)
- Micro‑USB cable
- Optional: External Wi‑Fi antenna (for lab testing)

---

## 💻 Software Requirements

- Arduino IDE
- ESP8266 Board Package
- USB driver (CH340 / CP210x)

---

## 🌐 Supported Devices

- Android smartphones
- iOS devices
- Windows
- Linux
- macOS

---

## 🧠 Recommended Knowledge

- Basic Arduino IDE usage
- Basic understanding of Wi‑Fi networks
- How to upload sketches to ESP boards

---

## ⚙️ Installation

### 📂 Clone the Repository
```bash
git clone https://github.com/yourusername/CredSniper
cd CredSniper

💾 Upload the Code

1. Open CredSniper.ino in Arduino IDE


2. Select NodeMCU 1.0 (ESP‑12E Module)


3. Set Flash Size to 4MB


4. Click Upload




---

🌐 Device Access

After flashing the firmware:

Wi‑Fi SSID : CredSniper
Password  : dewdew5218
Web Panel : http://192.168.4.1


---

🎯 Use Cases

Wi‑Fi security learning

Cybersecurity education

Authorized penetration testing

Security awareness demonstrations

ESP8266 experimentation



---

🙏 Credits

Spacehuhn — ESP8266 Deauther & Wi‑Fi research

ESP8266 open‑source community


This project is inspired by existing open‑source research and tools.


---

⚠️ Disclaimer

CredSniper is intended for educational and authorized security testing only.

Use only on networks you own

Or networks you have explicit permission to test


The developer is not responsible for misuse, damage, or illegal activity.


---

📜 License

This project is released for educational purposes.
See the LICENSE file for more information.

---

