# CredSniper
Custom ESP8266 framework for Wi-Fi networking and security research, focused on stability, embedded web control, and constrained-device optimization.
# ESP8266 Wi-Fi Security Research Framework

A custom embedded framework for **Wi-Fi networking and security research** built on the ESP8266 platform, with a focus on system integration, stability, and constrained-device optimization.

> ⚠️ **Disclaimer:**  
> This project is intended strictly for **educational and security research purposes** and must only be used in controlled lab environments or on networks you own or have explicit permission to test.

---

## 📌 Project Overview

This project explores low-level Wi-Fi behavior on the ESP8266 by integrating multiple networking components into a single, long-running embedded system.  
It was designed to push the ESP8266 close to its hardware limits while maintaining stability, responsiveness, and responsible usage.

The framework emphasizes:
- Embedded systems design under limited RAM and CPU resources
- Wi-Fi protocol behavior and packet-level experimentation
- Non-blocking task execution and memory-safe programming
- Embedded web interfaces for device control and monitoring

---

## ✨ Features (High-Level)

- Wi-Fi network discovery and monitoring  
- Packet-level Wi-Fi testing (research-oriented)  
- Beacon frame simulation  
- Captive portal workflow with DNS redirection  
- Embedded web-based admin interface  
- SPIFFS-based flash storage for data logging  
- Transmit power configuration and runtime tuning  
- Optimized task pacing for extended runtime stability (~90 minutes)

---

## 🧠 Technical Focus Areas

- Embedded Systems Programming (Arduino / C++)
- ESP8266 Wi-Fi stack behavior
- Memory management on constrained hardware
- Cooperative multitasking and non-blocking design
- Embedded HTTP and DNS services
- Flash-based file systems (SPIFFS)

---

## 🛠 Hardware Platform

- ESP8266 (NodeMCU / ESP-12 variants)
- External Wi-Fi antenna support
- Battery-powered operation tested

---

## 📊 Stability & Performance

The framework was tested in isolated lab conditions and demonstrated **approximately 90 minutes of continuous operation** under active workloads, highlighting careful memory handling and task scheduling on ESP8266 hardware.

---

## 📁 Project Scope

This project is **not** intended to bypass modern security protections or act as a real-world attack tool.  
Its purpose is to:
- Study Wi-Fi behavior
- Understand embedded networking constraints
- Practice responsible security research techniques

---

## 🙏 Acknowledgements

This project was inspired by and learned from existing open-source Wi-Fi research tools, particularly the work of **Spacehuhn Technologies**.

- **ESP8266 Deauther** by Spacehuhn  
  https://github.com/spacehuhntech/esp8266_deauther

Their projects and documentation were valuable resources for understanding ESP8266 Wi-Fi behavior and packet-level experimentation.  
This framework is an **independent implementation** developed for educational purposes.

---

## ⚠️ Legal & Ethical Notice

Unauthorized use of Wi-Fi testing tools against networks or devices without permission may be illegal in your jurisdiction.  
The author assumes **no responsibility** for misuse of this software.

Use responsibly.

---

## 👤 Author

**Dawood Khan**  
Embedded Systems & Networking Enthusiast  

---

## 📄 License

This project is released for educational use.  
Refer to included license information and ensure compliance with any third-party libraries used.
