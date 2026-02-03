🎯 CredSniper v3.0 - WiFi Security Testing Tool

⚠️ LEGAL DISCLAIMER: This tool is for AUTHORIZED SECURITY TESTING ONLY. Use only on networks you own or have explicit permission to test. Unauthorized use is illegal and unethical.

📋 Overview

CredSniper is a sophisticated ESP8266-based WiFi security assessment tool that demonstrates common wireless vulnerabilities in a controlled environment. It combines multiple attack vectors to test network security and user awareness.

✨ Features

🎣 Evil Twin Attack System

· Creates realistic fake access points mimicking popular services
· Hosts convincing login portals (Google, Instagram, Facebook, TikTok, Pinterest, Apple)
· Automatic credential capture and storage
· Auto-returns to admin mode after capture

💣 Deauthentication Attack

· Broadcast deauth targeting all clients simultaneously
· Per-client deauth for precise targeting
· Smart adaptive mode based on network conditions
· Channel-hopping for maximum coverage

🎵 Proven Beacon Spam (Spacehuhn Method)

· 70+ One Direction song title SSIDs for psychological impact
· Channel rotation (1, 6, 11) for full spectrum coverage
· Memory-optimized PROGMEM storage
· Stable 80+ minute runtime proven

📱 Professional Web Interface

· Responsive admin panel with real-time monitoring
· Template management system
· File upload for custom portals
· Credential viewing and download
· System health monitoring (RAM, storage, uptime)

🛠️ Technical Specifications

Hardware Requirements

· ESP8266 (NodeMCU, Wemos D1, etc.)
· Storage: 1MB+ SPIFFS recommended
· Power: USB or battery pack

Software Architecture

· Web Server: ESP8266WebServer with DNS hijacking
· Storage: SPIFFS for templates and credentials
· Packet Injection: wifi_send_pkt_freedom() for raw 802.11 frames
· Memory Management: PROGMEM + static buffers for stability

🚀 Quick Start

1. Installation

```bash
# Install PlatformIO or Arduino IDE
# Add ESP8266 board support
# Install required libraries:
# - ESP8266WiFi
# - ESP8266WebServer
# - DNSServer
# - FS (SPIFFS)

# Upload the code to your ESP8266
# Upload SPIFFS data (templates)
```

2. First Boot

1. Power on the ESP8266
2. Connect to WiFi: CredSniver (password: dewdew5218)
3. Access web interface: http://192.168.4.1
4. Configure your attack from the admin panel

📊 Attack Modes

Mode 1: Network Scanning

· Scans for available networks
· Displays signal strength, channels, and BSSIDs
· One-click target selection

Mode 2: Evil Twin Deployment

1. Select target network
2. Choose portal template
3. Start evil twin
4. Wait for credential capture
5. Automatic return to admin mode

Mode 3: Beacon Spam

· Floods area with 70+ fake networks
· Rotates channels 1, 6, 11
· Creates WiFi "noise" for testing client behavior

Mode 4: Deauth Attack

· Broadcast: Targets all clients
· Per-client: Precise MAC targeting
· Smart: Adaptive based on client count

🎭 Portal Templates

Built-in Templates:

· Google - "Your session has expired" prompt
· Instagram - "Session expired" notification
· Facebook - "Security alert" warning
· TikTok - "Login session timed out"
· Pinterest - "Authentication required"
· Apple - "Verification required"

Custom Templates:

· Upload HTML files via web interface
· Automatic template management
· Set as active with one click

📈 Performance Metrics

Stability:

· ✅ Beacon spam: 80+ minutes continuous (proven)
· ✅ Deauth: 24+ hours projected
· ✅ Web server: Stable under load

Effectiveness:

· Android: 95%+ disconnect rate
· iPhone: 90%+ disconnect rate
· Credential capture: Automatic on portal submission

Resource Usage:

· RAM: ~40KB free (healthy operation)
· SPIFFS: Templates + credential storage
· Power: Efficient for battery operation

🔧 Advanced Configuration

Memory Optimization

The tool uses several optimization techniques:

· PROGMEM for SSID storage (saves RAM)
· Static buffers (prevents heap fragmentation)
· File caching (30-second TTL for performance)

Customization Options

1. Edit ONE_DIRECTION_SSIDS array for custom beacon names
2. Modify portal templates in SPIFFS
3. Adjust timing intervals in code
4. Add new attack modes via web interface

📝 Credential Management

Storage Location:

/credentials.txt in SPIFFS

Format:

```
[Timestamp] Template: Service | User: username | Pass: password
```

Access Methods:

1. Web interface view
2. Direct download
3. Serial monitor logging

🛡️ Security Considerations

For Testers:

· Use in controlled environments only
· Obtain written authorization
· Document all testing activities
· Clear captured data after testing

For Defenders:

· Enable WPA3 with PMF (Management Frame Protection)
· Monitor for deauth floods
· Educate users about evil twins
· Implement certificate-based authentication

📚 Technical Details

Beacon Spam Implementation

```cpp
// Key optimizations:
const bool appendSpaces = true;  // Avoids dynamic allocation
const uint8_t channels[] = {1, 6, 11};  // Optimal channel hopping
// SSIDs stored in PROGMEM (flash, not RAM)
```

Deauth Packet Structure

```cpp
// Broadcast deauth format:
Destination: FF:FF:FF:FF:FF:FF  // All clients
Source:      AP's BSSID        // Appears from legitimate AP
BSSID:       AP's BSSID        // Network being impersonated
Reason:      0x0001            // Unspecified (widely accepted)
```

🐛 Known Issues & Solutions

Issue: Web UI slows after extended use

Solution: The system implements 30-second file caching. Manual cache invalidation available.

Issue: Memory fragmentation during long runs

Solution: Code uses static buffers and PROGMEM to minimize fragmentation.

Issue: Some clients ignore deauth

Solution: Enable per-client targeting or increase packet rate (200ms interval).

🤝 Contributing

Code Improvements Welcome:

1. Additional portal templates
2. Enhanced client detection
3. Better memory management
4. UI/UX improvements

Reporting Issues:

Please include:

· ESP8266 model
· Firmware version
· Steps to reproduce
· Serial output

📄 License

FOR EDUCATIONAL AND AUTHORIZED TESTING PURPOSES ONLY

This tool is provided "as-is" without warranty. Users assume all responsibility for legal and ethical use.

🙏 Credits

· Dawood Khan - Original development
· Spacehuhn - Beacon spam methodology
· ESP8266 Community - Hardware support
· Security Researchers - Testing and feedback

📞 Support

For legitimate security testing support:

· Review documentation thoroughly
· Test in isolated environments first
· Consult with legal counsel for authorization requirements

---

Remember: With great power comes great responsibility. Use this tool to improve security, not compromise it.

Last Updated: 2024 | Version: 3.0 | Author: Dawood Khan
