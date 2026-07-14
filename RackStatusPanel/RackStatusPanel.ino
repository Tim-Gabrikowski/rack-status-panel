#define PIN        D4 
#define BTN_MET_PIN D3
#define BTN_DEV_PIN D2

#define NUMPIXELS 28 
#define DEVICE_COUNT 4
#define METRIC_COUNT 4
#define UPDATE_INTERVAL 500

#define START_INDEX_MET 0
#define START_INDEX_A 4
#define START_INDEX_DEV 14
#define START_INDEX_B 18

#include <Adafruit_NeoPixel.h>

#include <ESP8266WiFi.h>
#include <WiFiManager.h>   // https://github.com/tzapu/WiFiManager
#include <EEPROM.h>
#include <PubSubClient.h>

// --- EEPROM SETTINGS ---
#define EEPROM_SIZE 512
#define EEPROM_START 0   // start offset

// --- CONFIG VARIABLES ---
char mqtt_server[40]   = "";
char mqtt_port[6]      = "1883";
char mqtt_user[20]     = "";
char mqtt_pass[20]     = "";
char mqtt_topic[30]    = "status/rackdisplay";

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

uint8_t status[DEVICE_COUNT][METRIC_COUNT * 2] = {};

int sel_device = 0;
int sel_metric = 0;

long lastUpdate = millis();

// Save config to EEPROM
void saveConfig() {
  EEPROM.begin(EEPROM_SIZE);

  int addr = EEPROM_START;
  auto writeStr = [&](const char* str, int maxLen) {
    for (int i = 0; i < maxLen; i++) {
      char c = (i < strlen(str)) ? str[i] : 0;
      EEPROM.write(addr++, c);
    }
  };

  writeStr(mqtt_server, sizeof(mqtt_server));
  writeStr(mqtt_port, sizeof(mqtt_port));
  writeStr(mqtt_user, sizeof(mqtt_user));
  writeStr(mqtt_pass, sizeof(mqtt_pass));
  writeStr(mqtt_topic, sizeof(mqtt_topic));

  EEPROM.commit();
}

// Load config from EEPROM
void loadConfig() {
  EEPROM.begin(EEPROM_SIZE);

  int addr = EEPROM_START;
  auto readStr = [&](char* buf, int maxLen) {
    for (int i = 0; i < maxLen; i++) {
      buf[i] = EEPROM.read(addr++);
    }
    buf[maxLen - 1] = 0;  // ensure null termination
  };

  readStr(mqtt_server, sizeof(mqtt_server));
  readStr(mqtt_port, sizeof(mqtt_port));
  readStr(mqtt_user, sizeof(mqtt_user));
  readStr(mqtt_pass, sizeof(mqtt_pass));
  readStr(mqtt_topic, sizeof(mqtt_topic));
}

// --- Ensure MQTT connection (reconnect if needed) ---
bool ensureMqttConnected() {
  if (client.connected()) return true;

  Serial.print("Connecting to MQTT...");

  if (client.connect("D1MiniClient", mqtt_user, mqtt_pass)) {
    Serial.println("connected!");

    client.subscribe(mqtt_topic);

    Serial.printf("Subscribed to %s", mqtt_topic);

    return true;
  } 
  else {
    Serial.print("failed, rc=");
    Serial.println(client.state());
    return false;
  }
}

bool publishMqttMessage(const char* subtopic, const char* message) {
  if (!ensureMqttConnected()) return false;

  char topic[100];
  snprintf(topic, sizeof(topic), "dev/%s/%s", mqtt_user, subtopic);

  bool result = client.publish(topic, message);
  if (result) {
    Serial.printf("MQTT -> [%s]: %s\n", topic, message);
  } else {
    Serial.println("MQTT publish failed!");
  }
  return result;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("MQTT <- ");
  Serial.println(topic);

  if (strcmp(topic, mqtt_topic) != 0) {
    return;
  }

  const int expectedSize = DEVICE_COUNT * METRIC_COUNT * 2;

  if (length != expectedSize) {
    Serial.printf("Invalid status size: %d (expected %d)\n", length, expectedSize);
    return;
  }

  int index = 0;

  for (int d = 0; d < DEVICE_COUNT; d++) {
    for (int m = 0; m < METRIC_COUNT * 2; m++) {
      status[d][m] = payload[index++];
    }
  }

  Serial.println("Status updated from MQTT");
}

void setupMqtt() {
  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(mqttCallback);
}

void init_status() {
  for(int d; d < DEVICE_COUNT; d++){
    for(int m; m < METRIC_COUNT * 2; m += 2){
      status[d][m] = (uint8_t)30;
      status[d][m+1] = (uint8_t)35;
    }
  }
}
void set_metrics() {
  if(millis() - lastUpdate < UPDATE_INTERVAL) return;

  for(int d = 0; d < DEVICE_COUNT; d++){
    for(int m = 0; m < METRIC_COUNT * 2; m += 2){
      status[d][m] = (uint8_t)random(0, 100);
      status[d][m+1] = (uint8_t)random(0, 100);
    }
  }
  lastUpdate = millis();
}

void display_selection(){
  pixels.setPixelColor(START_INDEX_DEV + sel_device, pixels.Color(25, 0, 0));
  pixels.setPixelColor(START_INDEX_MET + sel_metric, pixels.Color(25, 0, 0));
}
void display_value(int start, uint8_t value) {
  for(int i = 0; i < 10; i++) {
    if(value >= 10 + 10*i && i < 7) pixels.setPixelColor(start + i, pixels.Color(0, 25, 0));
    if(value >= 70 && i >= 7 && i <= 8) pixels.setPixelColor(start + i, pixels.Color(25, 25, 0));
    if(value >= 90 && i == 9) pixels.setPixelColor(start + i, pixels.Color(25, 0, 0));
  }
}
void display_metrics() {
  display_value(START_INDEX_A, status[sel_device][sel_metric]);
  display_value(START_INDEX_B, status[sel_device][sel_metric + 1]);
}

bool btn_dev_pressed = false;
bool btn_met_pressed = false;

void checkButtonPressed() {
  if(digitalRead(BTN_MET_PIN) == LOW && !btn_met_pressed) {
    btn_met_pressed = true;
    
    sel_metric++;
    sel_metric = sel_metric % METRIC_COUNT;
  } else if (digitalRead(BTN_MET_PIN) == HIGH) {
    btn_met_pressed = false;
  }

  if(digitalRead(BTN_DEV_PIN) == LOW && !btn_dev_pressed) {
    btn_dev_pressed = true;
    
    sel_device++;
    sel_device = sel_device % DEVICE_COUNT;
  } else if (digitalRead(BTN_DEV_PIN) == HIGH) {
    btn_dev_pressed = false;
  }
}

void setup() {
  pixels.begin();
  init_status();

  pinMode(BTN_MET_PIN, INPUT_PULLUP);
  pinMode(BTN_DEV_PIN, INPUT_PULLUP);

  /*
  * WIFI CONFIG AND WIFI MANAGER STUFF
  */
  // Load existing config
  loadConfig();

  // WiFiManager instance
  WiFiManager wm;

  // Button held? Reset settings
  if (digitalRead(BTN_MET_PIN) == LOW && digitalRead(BTN_DEV_PIN) == LOW) {
    Serial.println("Button pressed -> clearing WiFi settings");
    wm.resetSettings();
  }

  // Add custom fields for MQTT
  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "MQTT Username", mqtt_user, 20);
  WiFiManagerParameter custom_mqtt_pass("pass", "MQTT Password", mqtt_pass, 20);
  WiFiManagerParameter custom_mqtt_topic("topic", "MQTT Topic", mqtt_topic, 30);

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_pass);
  wm.addParameter(&custom_mqtt_topic);

  // Auto connect or start AP
  if (!wm.autoConnect("MyDevice-Setup", "12345678")) {
    Serial.println("Failed to connect and hit timeout");
    ESP.restart();
    delay(1000);
  }

  // Save updated config values
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port,   custom_mqtt_port.getValue());
  strcpy(mqtt_user,   custom_mqtt_user.getValue());
  strcpy(mqtt_pass,   custom_mqtt_pass.getValue());
  strcpy(mqtt_topic,   custom_mqtt_topic.getValue());

  saveConfig();

  // Connected to WiFi
  Serial.println("Connected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Print MQTT config
  Serial.println("MQTT Configuration:");
  Serial.printf("Server: %s\n", mqtt_server);
  Serial.printf("Port: %s\n", mqtt_port);
  Serial.printf("User: %s\n", mqtt_user);
  Serial.printf("Pass: %s\n", mqtt_pass);
  Serial.printf("Topic: %s\n", mqtt_topic);

  setupMqtt();
}

void loop() {
  ensureMqttConnected();
  client.loop();

  // set_metrics();
  
  checkButtonPressed();

  pixels.clear();

  display_selection();
  display_metrics();

  pixels.show();
}