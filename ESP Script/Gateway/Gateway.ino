/* Embedded SmartHome - Gateway
 *
 */
#include <esp_now.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include "AdafruitIO_WiFi.h"

uint8_t gatewayAddress[] = { 0xA8, 0x42, 0xE3, 0x47, 0x72, 0x3C };  // Node Controller MAC Address

esp_now_peer_info_t peerInfo;

#define LED_PIN 2

char IO_USERNAME[64] = "sonnguyen19";
char IO_KEY[64] = "aio_Wugb20f6BkjfQVN9rUpORDYubSEi";

#define WIFI_SSID "Relax & Drink"     
#define WIFI_PASS "14022001" 

AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
AdafruitIO_Feed *cambien1 = io.feed("cambien1");
AdafruitIO_Feed *cambien2 = io.feed("cambien2");
AdafruitIO_Feed *nutnhan1 = io.feed("nutnhan1");
AdafruitIO_Feed *nutnhan2 = io.feed("nutnhan2");
AdafruitIO_Feed *res = io.feed("res");
AdafruitIO_Feed *canhbao = io.feed("canhbao");

WiFiManager wifiManager;


void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? " -> Success" : " -> Fail");
  if (status != ESP_NOW_SEND_SUCCESS) {
    digitalWrite(LED_PIN, HIGH);
  }
  else digitalWrite(LED_PIN, LOW);
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  String data;
  String strs[20];
  int StringCount = 0;
  memcpy(&data, incomingData, sizeof(data));
  Serial.print("Data received: ");
  Serial.println(data);
  if (!data.startsWith("!") && !data.endsWith("#"))
    return;

  data.replace("!", "");
  data.replace("#", "");

  // Split the string into substrings
  while (data.length() > 0) {
    int index = data.indexOf(':');
    if (index == -1)  // No space found
    {
      strs[StringCount++] = data;
      break;
    } else {
      strs[StringCount++] = data.substring(0, index);
      data = data.substring(index + 1);
    }
  }

  // Show the resulting substrings
  // for (int i = 0; i < StringCount; i++) {
  //   Serial.print(i);
  //   Serial.print(": \"");
  //   Serial.print(strs[i]);
  //   Serial.println("\"");
  // }

  if (strs[1] == "TEMP") {
    cambien1->save(strs[2]);
  } else if (strs[1] == "HUMI") {
    cambien2->save(strs[2]);
  } else if (strs[1] == "LED") {
    nutnhan1->save(strs[2]);
  } else if (strs[1] == "DOOR") {
    nutnhan2->save(strs[2]);
  } else if (strs[1] == "WARN") {
    canhbao->save(strs[2]);
  } else if (strs[1] == "RES") {
    res->save(strs[2]);
  } 
}

void esp_now_init_func() {
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // Register for Send CallBack
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, gatewayAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  // Register for Recv CallBack
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("Finish init ESP NOW");
}

void send_now_data(String type, int data) {
  String ingoing_data = "!1:" + type + ":" + String(data) + "#";
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(gatewayAddress, (uint8_t *)&ingoing_data, sizeof(ingoing_data));

  if (result == ESP_OK) {
    Serial.print("Data send: ");
    Serial.print(ingoing_data);
  } else {
    Serial.println("Error sending the data");
  }
}



void handleMessage1(AdafruitIO_Data *data) {

  Serial.print("nutnhan1 <- ");
  Serial.println(data->toString());
  send_now_data("LED", data->toInt());

} 

void handleMessage2(AdafruitIO_Data *data) {

  Serial.print("nutnhan2 <- ");
  Serial.println(data->toString());
  send_now_data("DOOR", data->toInt());

} 

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

void setup()

{
  Serial.begin(9600);  // Initialize serial port for debugging.
  pinMode(LED_PIN, OUTPUT); 
  digitalWrite(LED_PIN, HIGH);
  WiFi.mode(WIFI_MODE_APSTA);

  int32_t channel = getWiFiChannel(WIFI_SSID);
  Serial.print("Wifi channel: ");
  Serial.println(channel);
  esp_wifi_set_channel(0, WIFI_SECOND_CHAN_NONE);

  Serial.printf("Connecting to Adafruit IO with User: %s / Key: %s.\n", IO_USERNAME, IO_KEY);
  io.connect();

  nutnhan1->onMessage(handleMessage1);
  nutnhan1->get();

  nutnhan2->onMessage(handleMessage2);
  nutnhan2->get();
  // wait for a connection

  while ((io.status() < AIO_CONNECTED)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Connected to Adafruit IO.");

  esp_now_init_func();
  digitalWrite(LED_PIN, LOW);
  
}  

int checkNode_counter = 0;

void checkNode(){
  if (checkNode_counter < 100) checkNode_counter+=1;
  else {
    checkNode_counter = 0;
    send_now_data("NODE", 1);
  }
}

void loop() {
  io.run();
  checkNode();
  delay(10);
} 
