#include <Arduino.h>
#include <WiFiNINA.h>
#include <config.h>

//digital slots
int LED_SLOT = 2;
int BUZZER_SLOT = 4;

//analog slots
int SOUND_SLOT = A0;
int LIGHT_SLOT = A1;
int TEMP_SLOT = A2;
int ROTARY_SLOT = A3;

// sensor values
int sound_val;
int light_val;
int touch_val;
int temp_val;
int rotary_val;
int temp_c; // temp converted to celsius


// const for temp conversion
const int B = 4275;
const int R0 = 100000;

// internet related stuff
IPAddress ip_address;
WiFiClient wifi_client;
char TS_SERVER[] = THINGSPEAK_API_SERVER;
String TS_API_KEY = THINGSPEAK_API_KEY;


// adjustable parameters
int GLOBAL_BUZZER_DELAY = 1000; //ms
int SOUND_THRESHOLD = 750;
int bulbBrightness = 1;


void connectWIFI() {
  analogWrite(LED_SLOT, LOW);
  Serial.println("Attempting connection to WIFI...");
  int retryNum = 0;

  //notify connection attempt
  while (WiFi.begin(WIFI_SSID, WIFI_PASSWORD) != WL_CONNECTED) {
    analogWrite(LED_SLOT, bulbBrightness);
    delay(500);
    retryNum++;
    Serial.println("Retrying attempt for WIFI connection...");
    analogWrite(LED_SLOT, LOW);
    delay(500);
  }

  Serial.println("Connected to WIFI!");

  ip_address = WiFi.localIP();
  Serial.println("Connected to IP: ");
  Serial.print(ip_address);
  
  analogWrite(LED_SLOT, bulbBrightness);
}


void checkAndReconnectWIFI() {
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Disconnected from WIFI! Retrying connection...");
    connectWIFI();
  }
}


void sendData(String field, int value) {
  String data = field + "=" + String(value);
  if (wifi_client.connect(TS_SERVER, 80)) {
    String url = "/update?api_key=" + TS_API_KEY + "&" + data;
    Serial.println("Sending data " + data + " to " + String(TS_SERVER) + "...");
    wifi_client.println("GET " + url + " HTTP/1.1");
    wifi_client.println("Host: " + String(TS_SERVER));
    wifi_client.println("Connection: close");
    wifi_client.println();
    delay(500);
    wifi_client.stop();
    Serial.println("Send complete!");
  }
}


// Buzzer
void triggerAlarm(int buzzerSlot=BUZZER_SLOT, int frequency=1000) {
  tone(buzzerSlot, frequency);
  delay(GLOBAL_BUZZER_DELAY);
  sendData("field1", 1); //alert
  noTone(BUZZER_SLOT);
}


// Sound sensor
void checkSound(int threshold, int inputValue) {
  if(inputValue > threshold) {
    triggerAlarm();
    sendData("field2", inputValue);
  }
  delay(500);
}


// Convert temperature input to celsius
float convertTemp(int inputValue) {
  float R = (1023.0 / inputValue - 1.0) * R0;
  float temperature_c = 1.0 / (log(R/R0) / B+1 / 298.15) - 273.15;
  Serial.print("Converted temperature result (celsius) = ");
  Serial.println(temperature_c);
  return temperature_c;
}


// Temperature sensor
void checkTempThreshold(int lower_threshold, int upper_threshold, int temp_c) {
  if(temp_c < lower_threshold || temp_c > upper_threshold) {
    sendData("field3", temp_c);
  }
  delay(500);
}


// Periodic temperature checks
void periodicTempUpdate(int temp_c, int interval) {
  int uptime = (int) (millis() / 1000);
  if(uptime % interval == 0) {
    sendData("field4", temp_c);
  }
  delay(500);
}


// Rotary potentiometer (for LED brightness adjustment)
void adjustLEDBightness(int rotary_val, int REF_VOLTAGE = 5, int FULL_ANGLE = 300, int GROVE_VCC = 5) {
  float voltage = (float) (rotary_val + 0.5) * REF_VOLTAGE / 1024.0;
  float degrees = (voltage * FULL_ANGLE) / GROVE_VCC;
  // bulbBrightness = map(degrees, 0, FULL_ANGLE, 0, 255);
  bulbBrightness = int(degrees / FULL_ANGLE * 255);
  analogWrite(LED_SLOT, bulbBrightness);
}


void setup() {
  Serial.begin(9600); 
  pinMode(LED_SLOT, OUTPUT);
  digitalWrite(LED_SLOT, LOW);
  pinMode(BUZZER_SLOT, OUTPUT);
  connectWIFI();
}


void loop() {
  checkAndReconnectWIFI();
  sound_val = analogRead(SOUND_SLOT);
  light_val = analogRead(LIGHT_SLOT);
  temp_val = analogRead(TEMP_SLOT);
  rotary_val = analogRead(ROTARY_SLOT);

  temp_c = convertTemp(temp_val);

  periodicTempUpdate(temp_c, 600);
  adjustLEDBightness(rotary_val);
  checkSound(SOUND_THRESHOLD, sound_val);
  checkTempThreshold(0, 50, temp_c);
}