🔐 CredSniper v3.0

<p align="center">
  <img src="https://raw.githubusercontent.com/spacehuhntech/esp8266_deauther/master/images/logo.png" width="150" alt="CredSniper Logo">
</p>

ESP8266 WiFi Security Assessment Tool
Inspired by Spacehuhn's ESP8266 Deauther

---

⚠️ Legal Notice

FOR AUTHORIZED SECURITY TESTING ONLY
Use only on networks you own or have explicit permission to test. Unauthorized access is illegal.

---

🚀 Quick Setup

1. Arduino IDE Setup

1. Install Arduino IDE
2. File → Preferences → Add URL:
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Tools → Boards Manager → Install "esp8266"

2. Upload Code

1. Open CredSniper.ino
2. Select Board: NodeMCU 1.0
3. Flash Size: 4MB (FS:1MB)
4. Port: Select your COM port
5. Click Upload

3. Connect to Device

1. Power ESP8266
2. WiFi: CredSniper (Password: dewdew5218)
3. Browser: 192.168.4.1

---

✨ Core Features

🎣 Evil Twin System

· Fake login portals (Google, Instagram, Facebook, TikTok, Pinterest, Apple)
· Auto credential capture
· Returns to admin after capture

💣 Deauth Attack

· 200ms timing for maximum effectiveness
· Broadcast & targeted modes
· Channel synchronized

🎵 Beacon Spam

· 70+ One Direction SSIDs
· Channel hopping (1,6,11)
· Stable long-term operation

📱 Web Control Panel

· Real-time monitoring
· Network scanner
· File management
· Easy to use interface

---

📸 Screenshot

<p align="center">
  <img src="https://raw.githubusercontent.com/spacehuhntech/esp8266_deauther/master/screenshots/webif.png" width="600" alt="Web Interface">
</p>

---

🙏 Credits

<p align="center">
  <a href="https://github.com/spacehuhntech">
    <img src="https://raw.githubusercontent.com/spacehuhntech/brand/master/logo/spacehuhn_logo_light.svg" width="100" alt="Spacehuhn">
  </a>
  <br>
  Special thanks to <b>Spacehuhn</b> for the ESP8266 Deauther project
</p>

· Spacehuhn - Beacon spam methodology and ESP8266 Deauther inspiration
· ESP8266 Community - Hardware support and libraries
· Security Researchers - Testing and feedback

This tool builds upon proven techniques from Spacehuhn's security research.

---

❓ Quick Help

· No WiFi AP? Check serial monitor (115200 baud)
· Upload fails? Install CH340 drivers
· Web not loading? Clear browser cache
· Weak signal? Use external antenna

---

📄 License

Educational & Authorized Security Research Only

Use responsibly to improve security, not compromise it.

---

<p align="center">
  <b>CredSniper v3.0</b><br>
  <i>Inspired by Spacehuhn's work | For Security Education</i>
</p>
