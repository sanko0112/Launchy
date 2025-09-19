#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <OneWire.h>
#include <LaunchDisplayLibrary.h>
#include <esp_wifi.h>

//defines
#define iButton 3
#define togglesw 0
#define GREEN_LED 5
#define RED_LED 6

uint8_t launch_code;
int session_id;
//mac address for Receiver
uint8_t broadcastAddress[] = {0x34, 0xCD, 0xB0, 0xE9, 0x1C, 0xBC}; //34:CD:B0:E9:1C:BC

//iButton read declarations
OneWire ds(iButton);
byte storedID[8];
byte allowedID[8] = {0x01, 0x6E, 0x83, 0x1D, 0x01, 0x00, 0x00, 0x66}; // iButton accepted ID  
 // iButton accepted ID  
uint8_t EN; // EN flag

esp_now_peer_info_t peerInfo;

uint8_t countdownTime = 5;
const int T_minValue = 3;
const int T_maxValue = 20;
bool countdown_confirmed = false;

uint8_t preheatStartTime = 3;
const int P_SminValue = 1;
int P_SmaxValue = 5;
bool preheat_start_confirmed = false;

float preheatDuration = 0.25;
const float durationResolution = 0.25;
const float P_DminValue = 0.25;
float P_DmaxValue = 5.0;
bool preheat_confirmed = false;

uint8_t ignitionDelta = 5;
const int I_DminValue = 1;
const int I_DmaxValue = 10;
bool ignitionDelta_confirmed = false;

uint8_t batt_type = 1;
bool batt_confirmed = false;

typedef struct struct_message {
uint8_t packet_type;
uint8_t batt_type;
uint8_t launch_code;
uint8_t countdownTime;
uint8_t preheatStartTime;
float preheatDuration;
float durationResolution;
uint8_t ignitionDelta;
int session_id;
} struct_message;

struct_message LaunchConfig;

typedef struct {
  uint8_t packet_type;
  bool continuity_result;
} continuity_response_t;

#define PACKET_TYPE_LAUNCH     0
#define PACKET_TYPE_CONTINUITY 1

uint8_t continuityTest_init = 5;
bool continuity_result = false;
bool recvFlag = false;
//function prototypes
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void HandleSingleButton(void (*onShortPress)(), void (*onLongPress)());
void WaitForLaunchConfirm();
void iButtonAuth();
void blink(uint8_t pin, uint16_t times, uint16_t ms);
void reverseblink(uint8_t pin, uint16_t times, uint16_t ms);
void auth();

void setup() {

  Serial.begin(115200);
   Wire.begin(7, 8);
  u8g2.begin();
  pinMode(GREEN_LED, OUTPUT); // Green_LED indicator
  pinMode(RED_LED, OUTPUT); // RED_LED indicator
  pinMode(togglesw, INPUT_PULLDOWN); // Toggle switch as input

  WiFi.mode(WIFI_STA);
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);  // Long Range mode


  blink(GREEN_LED, 3, 100);
  delay(50);
  digitalWrite (RED_LED, HIGH);

  // Initilize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register the send callback
  esp_now_register_send_cb(OnDataSent);
  // In setup() or once before any send
  esp_now_register_recv_cb(OnDataRecv);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  draw_welcome();
  delay(1500);
  loading();
}
 
void loop() {
  //checking for valid ibutton
  auth();
  delay(10);
  //valid ibutton found
  if(EN){

    digitalWrite (RED_LED, LOW);
    digitalWrite (GREEN_LED, HIGH);
    draw_auth_success();
    delay(1500);
    
    //adding ESP-NOW peer
    if (!esp_now_is_peer_exist(broadcastAddress)) {
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
      }
      else{
        Serial.println(" added peer");
    }
    }
    delay(100);
    openBatteryTypeMenu();
    if (batt_type == 1)
    { 
    continuity();
    s_settings();
    }
    else
    {
      ss_settings();
    }
    draw_armed();
    LaunchConfig.packet_type = PACKET_TYPE_LAUNCH;
    launch_code = 33;
    session_id = rand();
    LaunchConfig.launch_code = launch_code;
    LaunchConfig.countdownTime = countdownTime;
    LaunchConfig.preheatStartTime = preheatStartTime;
    LaunchConfig.preheatDuration = preheatDuration;
    LaunchConfig.durationResolution = durationResolution;
    LaunchConfig.ignitionDelta = ignitionDelta;
    LaunchConfig.session_id = session_id;
    delay(2000);
    //Waiting for button press
    WaitForLaunchConfirm();
    //sending launch command

    for(int j = 0; j<3; j++){
      LaunchConfig.packet_type = PACKET_TYPE_LAUNCH;
      esp_now_send(broadcastAddress, (uint8_t *)&LaunchConfig, sizeof(LaunchConfig));
    }
    //starting countdown
    countdown(countdownTime,preheatStartTime,preheatDuration);
    draw_ignition();
    delay(10);
    delay(ignitionDelta*1000);
    esp_now_del_peer(broadcastAddress); // Optional: prevent re-use until re-auth
    digitalWrite (GREEN_LED, LOW);
    delay(20);
    digitalWrite (RED_LED, HIGH);
    EN = 0;
  }
  //ibutton auth failed retrying...
  else{
    auth();
  }
}

void OnDataSent(const uint8_t *info, esp_now_send_status_t status) {
  (void)info;
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void iButtonAuth()
{
  //initializing
  byte addr[8];
  if (!ds.search(addr)) {
    ds.reset_search();
    return;
  }
  //checking iButton type
  if (OneWire::crc8(addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return;
  }
  //storing recieved ID
  for (int i = 0; i < 8; i++) {
    storedID[i] = addr[i];
  }
  //checking if recieved ID is matching the stored ID
  bool isAuth = true;
  for (int i = 0; i < 8; i++) {
    if (storedID[i] != allowedID[i]) {
      isAuth = false;
    }
  }
  //matching ID EN flag TRUE
  if (isAuth) {
    Serial.println(" -> AUTHORIZED");
    EN = 1;
  }
  //mismatch on ID's EN flag stays FALSE
  else {
    Serial.println("FAILED");
    reverseblink(RED_LED, 2, 200);
    EN = 0;
  }
  delay(200);
}

void HandleSingleButton(void (*onShortPress)(), void (*onLongPress)()) {
  const unsigned long longPressTime = 1000; // 1 second hold
  unsigned long pressStart = 0;
  bool isPressed = false;
  bool longPressTriggered = false;

  while (true) {
    int state = digitalRead(togglesw);

    if (state == HIGH) {
      if (!isPressed) {
        pressStart = millis();
        isPressed = true;
      } else if (!longPressTriggered && (millis() - pressStart >= longPressTime)) {
        longPressTriggered = true;
        onLongPress();  // Immediately trigger long press
        break;
      }
    } else if (state == LOW && isPressed) {
      // Button released before long press time
      if (!longPressTriggered) {
        onShortPress();  // Short press handler
      }
      break;
    }

    delay(10);
  }
}

void WaitForLaunchConfirm() {
  Serial.println("Hold to launch...");
  HandleSingleButton([]() {}, []() {
    Serial.println("Launch confirmed.");
    // Proceed with countdown + ignition
  });
}

void blink(uint8_t pin, uint16_t times, uint16_t ms)
{
  for (uint16_t i = 0; i< times; i++){
    digitalWrite(pin, HIGH);
    delay(ms);
    digitalWrite(pin, LOW);
    delay(ms);
  }
}

void reverseblink(uint8_t pin, uint16_t times, uint16_t ms)
{
  for (uint16_t i = 0; i< times; i++){
    digitalWrite(pin, LOW);
    delay(ms);
    digitalWrite(pin, HIGH);
    delay(ms);
  }
}


void auth()
{ 
  while(1){
    draw_auth_a();
    iButtonAuth();
    delay(100);
    draw_auth_b();
    iButtonAuth();
    delay(100);
    draw_auth_c();
    iButtonAuth();
    delay(100);
    if(EN)
      break;
    draw_auth_d();
    iButtonAuth();
    delay(100);
    if(EN)
      break;    
    draw_auth_e();
    iButtonAuth();
    delay(100);
    if(EN)
      break;    
    draw_auth_f();
    iButtonAuth();
    delay(100);
    if(EN)
      break;    
    draw_auth_g();
    
    iButtonAuth();
    delay(100);
    if(EN)
      break;
  }   
}

void updateCountdownText() {
  itoa(countdownTime, countdown_menu_var, 10);
}

void onShortPress_CountdownMenu() {
  countdownTime++;
  if (countdownTime > T_maxValue) countdownTime = T_minValue;
  updateCountdownText();
  draw_countdown_menu();
}

void onLongPress_CountdownMenu() {
  countdown_confirmed = true;
  Serial.print("Countdown set to: ");
  Serial.println(countdownTime);
}

void openCountdownMenu() {
  countdown_confirmed = false;
  updateCountdownText();
  draw_countdown_menu();

  while (!countdown_confirmed) {
    HandleSingleButton(onShortPress_CountdownMenu, onLongPress_CountdownMenu);
  }
}


void updatePreheatStartText() {
  itoa(preheatStartTime, preheat_start_menu_var, 10);
}

void onShortPress_PreheatStartMenu() {
  preheatStartTime++;
  if (preheatStartTime > P_SmaxValue) preheatStartTime = P_SminValue;
  updatePreheatStartText();
  draw_preheat_start_menu();
}

void onLongPress_PreheatStartMenu() {
  preheat_start_confirmed = true;
  Serial.print("Preheat start time set to: T-");
  Serial.println(preheatStartTime);
}

void openPreheatStartMenu() {
  preheat_start_confirmed = false;
  updatePreheatStartText();
  draw_preheat_start_menu();
  delay(400);

  while (!preheat_start_confirmed) {
    HandleSingleButton(onShortPress_PreheatStartMenu, onLongPress_PreheatStartMenu);
  }
}


void updatePreheatText() {
  dtostrf(preheatDuration, 3, 2, preheat_duration_menu_var);
}

void onShortPress_PreheatMenu() {
  preheatDuration += durationResolution;
  if (preheatDuration > P_DmaxValue) preheatDuration = P_DminValue;
  updatePreheatText();
  draw_preheat_duration_menu();
}

void onLongPress_PreheatMenu() {
  preheat_confirmed = true;
  Serial.print("Preheat set to: ");
  Serial.println(preheatDuration);
}

void openPreheatDurationMenu() {
  preheat_confirmed = false;
  updatePreheatText();
  draw_preheat_duration_menu();
  delay(400);
  while (!preheat_confirmed) {
    HandleSingleButton(onShortPress_PreheatMenu, onLongPress_PreheatMenu);
  }
}

void updateIgnitionDeltaText() {
  itoa(ignitionDelta, ignition_delta_menu_var, 10);
}

void onShortPress_IgnitionMenu() {
  ignitionDelta++;
  if (ignitionDelta > I_DmaxValue) ignitionDelta = I_DminValue;
  updateIgnitionDeltaText();
  draw_ignition_delta_menu();
}

void onLongPress_IgnitionMenu() {
  ignitionDelta_confirmed = true;
  Serial.print("Ignition Delta is set to: ");
  Serial.println(ignitionDelta);
}

void openIgnitionDeltaMenu() {
  ignitionDelta_confirmed = false;
  updateIgnitionDeltaText();
  draw_ignition_delta_menu();
  delay(400);
  while (!ignitionDelta_confirmed) {
    HandleSingleButton(onShortPress_IgnitionMenu, onLongPress_IgnitionMenu);
  }
}

void s_settings(void)
{
openCountdownMenu();
P_SmaxValue = countdownTime;
openPreheatStartMenu();
P_DmaxValue = preheatStartTime-0.75;
openPreheatDurationMenu();
openIgnitionDeltaMenu();

itoa(countdownTime, armed_t_, 10);
itoa(preheatStartTime, armed_ps, 10);
itoa(preheatDuration, armed_pd, 10);
itoa(ignitionDelta, armed_id, 10);
/*
int launch_code;
int countdownTime;
int preheatStartTime;
float preheatDuration;
int ignitionDelta;
*/
}
void ss_settings(void)
{
openCountdownMenu();
P_SmaxValue = countdownTime;
openIgnitionDeltaMenu();
itoa(countdownTime, armed_t_, 10);
itoa(preheatStartTime, armed_ps, 10);
itoa(preheatDuration, armed_pd, 10);
itoa(ignitionDelta, armed_id, 10);
/*
int launch_code;
int countdownTime;
int preheatStartTime;
float preheatDuration;
int ignitionDelta;
*/
}
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
    if (len == sizeof(continuity_response_t)) {
        continuity_response_t response;
        memcpy(&response, incomingData, len);
        if (response.packet_type == PACKET_TYPE_CONTINUITY) {
            continuity_result = response.continuity_result;
            Serial.print("Received continuity result: ");
            Serial.println(continuity_result);
        }
    }
  recvFlag = true;  
}


void continuity() {

  Serial.println("Press and hold to begin continuity test...");
  draw_continuity_check_start();
  HandleSingleButton([]() {}, []() {
    Serial.println("Continuity test started");

    struct_message continuity_packet = {};
    continuity_packet.packet_type = PACKET_TYPE_CONTINUITY;
    esp_now_send(broadcastAddress, (uint8_t *)&continuity_packet, sizeof(continuity_packet));
    unsigned long startTime = millis();
    while ( millis() - startTime < 1500) {
      delay(10); // yield time for callback
    }
        if (continuity_result) {
          Serial.println("Continuity success. Hold to continue...");
          draw_continuity_check_success();
          HandleSingleButton([]() {}, []() {
          Serial.println("Continuing...");
          delay(400);
          });
        } else {
          Serial.println("Continuity failed. Check connection and retry.");
          draw_continuity_check_failed();
          while(1){}
         // wait forever or implement retry
        }
     
  });
}

// ===== Battery type menu =====
// 1S = 1, 2S = 2


// Show the right screen for current selection
void draw_batt_current() {
  if (batt_type == 1) {
    draw_batt_s();   // your "1S selected" screen
  } else {
    draw_batt_ss();  // your "2S selected" screen
  }
}

// Short press: toggle 1S <-> 2S
void onShortPress_BattMenu() {
  batt_type = (batt_type == 1) ? 2 : 1;
  draw_batt_current();
}

// Long press: confirm selection
void onLongPress_BattMenu() {
  batt_confirmed = true;
  Serial.print("Battery type set to: ");
  Serial.println(batt_type == 1 ? "1S" : "2S");
}

// Open the menu and wait for confirmation
void openBatteryTypeMenu() {
  batt_confirmed = false;
  draw_batt_current();
  delay(300); // small debounce like your other menus

  while (!batt_confirmed) {
    HandleSingleButton(onShortPress_BattMenu, onLongPress_BattMenu);
  }
}