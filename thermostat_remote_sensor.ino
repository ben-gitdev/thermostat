/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-esp-now-wi-fi-web-server/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// Set your Board ID (ESP32 Sender #1 = BOARD_ID 1, ESP32 Sender #2 = BOARD_ID 2, etc)
#define BOARD_ID 1

// Digital pin connected to the DHT sensor
#define DHTPIN 4  

// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

//MAC Address of the receiver 
uint8_t broadcastAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
    int id;
    float temp;
    float hum;
    int readingId;
} struct_message;

//Create a struct_message called myData
struct_message myData;

unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 5000;        // Interval at which to publish sensor readings

unsigned int readingId = 0;

// Insert your SSID
constexpr char WIFI_SSID[] = "*****";//replace with your WIFI SSID

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

float temperature1 = 0;
float temperature2 = 0;
float temperature3 = 0;

float readDHTTemperature() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature(true);
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //float t = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again).
  if (isnan(t)) {    
    Serial.println("Failed to read from DHT sensor!");
    return 0;
  }
  else {
    Serial.println(t);
    return t;
  }
}

float readDHTHumidity() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return 0;
  }
  else {
    Serial.println(h);
    return h;
  }
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
 
void setup() {
  //Init Serial Monitor
  Serial.begin(115200);

  dht.begin();
 
  // Set device as a Wi-Fi Station and set channel
  WiFi.mode(WIFI_STA);

  int32_t channel = getWiFiChannel(WIFI_SSID);

  WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial); // Uncomment to verify channel change after

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  //Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.encrypt = false;
  
  //Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}
 
void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // Save the last time a new reading was published
    previousMillis = currentMillis;
    //Set values to send
    float temperatureCurrent = readDHTTemperature();
    temperature1 = temperature2;
    temperature2 = temperature3;
    temperature3 = temperatureCurrent;
    myData.temp = temperature3;
    Serial.println(temperature1);
    Serial.println(temperature2);
    Serial.println(temperature3);
    if(temperature1<=temperature2 && temperature2<=temperature3){
      myData.temp = temperature2;
      }
    if(temperature1<=temperature3 && temperature3<=temperature2){
      myData.temp = temperature3;
      }
    if(temperature2<=temperature1 && temperature1<=temperature3){
      myData.temp = temperature1;
      }
    if(temperature2<=temperature3 && temperature3<=temperature1){
      myData.temp = temperature3;
      }
    if(temperature3<=temperature1 && temperature1<=temperature2){
      myData.temp = temperature1;
      }
    if(temperature3<=temperature2 && temperature2<=temperature1){
      myData.temp = temperature2;
      } 
    Serial.println(myData.temp);
    myData.id = BOARD_ID;
    myData.hum = readDHTHumidity();
    myData.readingId = readingId++;
     
    //Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data,try to initialize ESP-NOW again");
	  // try to initialize ESP-NOW again
	  WiFi.mode(WIFI_STA);

	  int32_t channel = getWiFiChannel(WIFI_SSID);

	  WiFi.printDiag(Serial); // Uncomment to verify channel number before
	  esp_wifi_set_promiscuous(true);
	  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
	  esp_wifi_set_promiscuous(false);
	  WiFi.printDiag(Serial); // Uncomment to verify channel change after

	  //Init ESP-NOW
	  if (esp_now_init() != ESP_OK) {
		Serial.println("Error initializing ESP-NOW");
		return;
	  }

	  // Once ESPNow is successfully Init, we will register for Send CB to
	  // get the status of Trasnmitted packet
	  esp_now_register_send_cb(OnDataSent);
	  
	  //Register peer
	  esp_now_peer_info_t peerInfo;
	  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
	  peerInfo.encrypt = false;
	  
	  //Add peer        
	  if (esp_now_add_peer(&peerInfo) != ESP_OK){
		Serial.println("Failed to add peer");
		return;
	  }
    }
  }
}
