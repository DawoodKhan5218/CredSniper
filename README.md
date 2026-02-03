<div align="center">

# 🔐 CredSniper v3.0

### ESP8266 Wi‑Fi Security Assessment & Awareness Framework

![Platform](https://img.shields.io/badge/Platform-ESP8266-blue.svg)
![Framework](https://img.shields.io/badge/Framework-Arduino-00979D.svg)
![Purpose](https://img.shields.io/badge/Purpose-Security%20Testing-red.svg)
![Status](https://img.shields.io/badge/Status-Stable-brightgreen.svg)

<img src="https://iili.io/fZQSlIt.md.png" alt="CredSniper Logo" width="280"/>

</div>

CredSniper is a standalone ESP8266-based Wi‑Fi security testing tool designed to demonstrate real‑world wireless attack surfaces caused by human trust and misconfiguration.
It is inspired by Spacehuhn’s ESP8266 Deauther and extended with a modern web interface and modular attack simulation components.




✨ Features

Evil Twin Framework: Captive portal simulation using familiar login pages.

Deauthentication Engine: Wireless disruption testing for client reassociation behavior.

Beacon Spam Module: RF environment stress‑testing via SSID flooding.

Web-Based Control Panel: Fully browser‑controlled interface (no serial needed).

Standalone Operation: Runs entirely on ESP8266 without external hardware.

Long Runtime Stability: Designed for extended testing sessions.




🛠️ Requirements

Hardware

1. ESP8266 NodeMCU (ESP‑12E / ESP‑12F)


2. Micro‑USB Cable


3. Optional: External antenna (for controlled lab testing)



Software

1. Arduino IDE


2. ESP8266 Board Package





⚙️ Installation & Setup

Arduino IDE Configuration

Add the ESP8266 boards URL:

http://arduino.esp8266.com/stable/package_esp8266com_index.json

Then install:

Boards Manager → ESP8266 by ESP8266 Community




Flashing the Firmware

1. Open CredSniper.ino


2. Select:

Board: NodeMCU 1.0 (ESP‑12E Module)

Flash Size: 4MB



3. Upload the sketch






Device Access

Wi‑Fi SSID : CredSniper
Password  : dewdew5218
Web Panel : http://192.168.4.1




🧠 Use Cases

Security awareness demonstrations

Authorized wireless penetration testing

Educational labs & research environments

Defensive testing of Wi‑Fi deployments





🙏 Credits

This project is inspired by and built upon research by:

Spacehuhn — ESP8266 Deauther & Wi‑Fi security research

ESP8266 open‑source community


All credit for foundational techniques goes to their respective authors.




⚠️ Disclaimer

This project is intended for educational and authorized security testing only.

You must:

Own the network or

Have explicit written permission to test it


The author is not responsible for misuse, damage, or legal consequences.




📜 License

This project is released for educational use.
Refer to the repository license file for details.
