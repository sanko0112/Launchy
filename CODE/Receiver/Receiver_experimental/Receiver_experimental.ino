  #include <esp_now.h>
  #include <WiFi.h>
  #include <esp_wifi.h>

  // Pin definitions
  #define RELAY_PIN 2
  #define BUZZ 3
  #define ADC_PIN 0

  // Launch code value to match
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
  
  int lastSession_id = 4;

  typedef struct {
  uint8_t packet_type;
  uint8_t continuity_result;
  } continuity_response_t;

  uint8_t continuity_result = 0;

  #define PACKET_TYPE_LAUNCH     0
  #define PACKET_TYPE_CONTINUITY 1

  uint8_t broadcastAddress[] = {0x34, 0xCD, 0xB0, 0xE9, 0x1C, 0x48}; //34:CD:B0:E9:1C:48


  // Setup function
  void setup() {
    Serial.begin(115200);

    // Initialize pins
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUZZ, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);   // Relay OFF
    digitalWrite(BUZZ, LOW);  // Buzzer OFF

    // Set WiFi to station mode
    WiFi.mode(WIFI_STA);
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);  // Long Range mode


    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
      return;
    }
    esp_now_peer_info_t peer;
    memset(&peer, 0, sizeof(peer));
    memcpy(peer.peer_addr, broadcastAddress, 6);
    peer.channel = 0;
    peer.encrypt = false;

    if (!esp_now_is_peer_exist(peer.peer_addr)) {
      esp_err_t addStatus = esp_now_add_peer(&peer);
    if (addStatus != ESP_OK) {
      Serial.println("Failed to add transmitter as peer!");
  }
}
    // Register receive callback
    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("Receiver ready. Waiting for peer auth and launch command...");
    startup_sound();
  }

  // Loop does nothing, waiting for data via callback
  void loop() {
    delay(10);
  }

  // Callback function when data is received
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  struct_message msg;
  memcpy(&msg, incomingData, sizeof(msg));
  if (msg.packet_type == PACKET_TYPE_CONTINUITY) {
    int adcVal = 0;
    int freeValue = 5000;
    int loadValue = 5000;
    for (int q = 0; q<5; q++)
    {
      adcVal = analogRead(ADC_PIN);
      if (adcVal < freeValue){
        freeValue = adcVal;
      }
      delay(20);
    }
    digitalWrite(RELAY_PIN, HIGH);
    for(int p = 0; p<10;p++)
    {
      adcVal = analogRead(ADC_PIN);
      if (adcVal < loadValue){
        loadValue = adcVal;
      }
      delay(50);
    }
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("freeValue");
    Serial.println(freeValue);
    Serial.println("loadValue");
    Serial.println(loadValue);
    if(loadValue<=freeValue-20)
    {
      continuity_result = true;
    }
    else
    {
      continuity_result = false;
    }
    continuity_response_t response;
    response.packet_type = PACKET_TYPE_CONTINUITY;
    response.continuity_result = continuity_result;

    for (int k = 0; k < 5; k++) {
      esp_now_send(broadcastAddress, (uint8_t *)&response, sizeof(response));
    }
    Serial.println(continuity_result);
    if (continuity_result)
    {
       onReceive_sound(2);
    }
    else{fail_sound();}
    delay(100);
  }
  if (msg.packet_type == PACKET_TYPE_LAUNCH && msg.session_id != lastSession_id) {
    lastSession_id = msg.session_id;
    LaunchConfig = msg;
    uint8_t batteryType;
    batteryType = LaunchConfig.batt_type;
    Serial.println(batteryType);
    if (batteryType == 1) {
      TickType_t lastWakeTime = xTaskGetTickCount();
      int durationResolution_ms = LaunchConfig.durationResolution * 1000;
      int preheatDuration_ms = LaunchConfig.preheatDuration * 1000;
      int preheatStart_ms = LaunchConfig.preheatStartTime * 1000;
      int countdown_ms = LaunchConfig.countdownTime * 1000-250;     //countdown-onReceive_sound
      onReceive_sound(5);
      if (LaunchConfig.launch_code == 33) {
        for (int t = countdown_ms; t >= 0; t -= durationResolution_ms) {
        // Start preheat when time left <= preheatStart and there's preheat duration remaining
          if (t <= preheatStart_ms && preheatDuration_ms > 0) {
          digitalWrite(RELAY_PIN, HIGH);  // Relay ON (preheat)
          digitalWrite(BUZZ, HIGH);
          preheatDuration_ms -= durationResolution_ms;
          } else {
          digitalWrite(RELAY_PIN, LOW); // Relay OFF
          digitalWrite(BUZZ, LOW);
         }

          vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(durationResolution_ms));
        }

        // IGNITION PHASE
        digitalWrite(RELAY_PIN, HIGH);  // Relay ON (ignite)
        digitalWrite(BUZZ, HIGH);
        delay(LaunchConfig.ignitionDelta * 1000);
        digitalWrite(RELAY_PIN, LOW); // Relay OFF
        digitalWrite(BUZZ, LOW); // Relay OFF
      }
    }
    else
    {
      TickType_t lastWakeTime = xTaskGetTickCount();
      int countdown_ms = LaunchConfig.countdownTime * 1000-250;
      int durationResolution_ms = LaunchConfig.durationResolution * 1000;     //countdown-onReceive_sound
      onReceive_sound(5);
      if (LaunchConfig.launch_code == 33) {
        for (int t = countdown_ms; t >= 0; t -= durationResolution_ms) {
          vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(durationResolution_ms));
        }
      }
      // IGNITION PHASE
      digitalWrite(RELAY_PIN, HIGH);  // Relay ON (ignite)
      digitalWrite(BUZZ, HIGH);
      delay(LaunchConfig.ignitionDelta * 1000);
      digitalWrite(RELAY_PIN, LOW); // Relay OFF
      digitalWrite(BUZZ, LOW); // Relay OFF
    }
  }
}
 void startup_sound()
{
    digitalWrite(BUZZ, HIGH);
    delay(300);
    digitalWrite(BUZZ, LOW);
    delay(300);
    digitalWrite(BUZZ, HIGH);
    delay(75);
    digitalWrite(BUZZ, LOW);
    delay(75);
    digitalWrite(BUZZ, HIGH);
    delay(75);
    digitalWrite(BUZZ, LOW);
    delay(75);
}
  void onReceive_sound(int times)
  {
    for(int i = 0; i < times; i++)
    {
    digitalWrite(BUZZ, HIGH);
    delay(50);
    digitalWrite(BUZZ, LOW);
    delay(50);
    }
  }
void fail_sound()
{
    digitalWrite(BUZZ, HIGH);
    delay(300);
    digitalWrite(BUZZ, LOW);
    delay(300);
    digitalWrite(BUZZ, HIGH);
    delay(300);
    digitalWrite(BUZZ, LOW);
    delay(300);
    digitalWrite(BUZZ, HIGH);
    delay(300);
    digitalWrite(BUZZ, LOW);
    delay(300);
}