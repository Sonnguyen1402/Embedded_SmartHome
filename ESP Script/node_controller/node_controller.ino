/*   Node Controller
*
*/

#include <ezButton.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <esp_now.h>
#include <WiFi.h>
#include <string.h>
#include <esp_wifi.h>

#define INIT_PIN 2  // Button Pin 1, control LED

#define BUTTON1_PIN 16  // Button Pin 1, control LED
#define BUTTON2_PIN 17  // Button Pin 2, control door's motor

#define LED_RED_PIN 4     // Warning LED Pin
#define LED_YELLOW_PIN 5  // Door's Motor Pin
#define LED_GREEN_PIN 15  // LED Pin

#define DHTPIN 13      // DHT sensor Pin
#define DHTTYPE DHT11  // DHT 11 Type

#define IR_SENSOR_PIN 18  // IR obstacle avoidance sensor Pin

#define RS_DO_PIN 19  // Rain sensor DO Pin

ezButton button1(BUTTON1_PIN);  // create ezButton object
ezButton button2(BUTTON2_PIN);  // create ezButton object

DHT dht(DHTPIN, DHTTYPE);

// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;

// set LCD address, number of columns and rows
LiquidCrystal_I2C lcd(0x3F, lcdColumns, lcdRows);


int led_red_state = LOW;     // the current state of Warning LED
int led_yellow_state = LOW;  // the current state of Door's Motor LED
int led_green_state = LOW;   // the current state of LED

int led_red_counter = 0;
int led_yellow_counter = 0;

int door_status = LOW;  // the current state of Door

int dht_counter = 0;
int dht_humi = 0;
int dht_temp = 0;

int send_dht_data_counter = 0;
int temp_warning = 0;

int IR_lastState = HIGH;  // the previous state from the IR input pin
int IR_currentState;      // the current reading from the IR input pin
int IR_warning = 0;

int RS_lastState = HIGH;  // the previous state from the RS input pin
int RS_currentState;      // the current reading from the RS input pin
int RS_warning = 0;

uint8_t gatewayAddress[] = { 0x24, 0xDC, 0xC3, 0xA7, 0x4B, 0xE4 };  // Gateway MAC Address

esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? " -> Success" : " -> Fail");
  if (status != ESP_NOW_SEND_SUCCESS) {
    digitalWrite(INIT_PIN, HIGH);
  }
  else digitalWrite(INIT_PIN, LOW);
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  String data;
  memcpy(&data, incomingData, sizeof(data));
  Serial.print("Data received: ");
  Serial.println(data);
  // Turn off led
  if (data == "!1:LED:0#") {
    if (led_green_state == HIGH)
      toggle_led_green();
    else send_now_data("RES", 10);
  // Turn on led
  } else if (data == "!1:LED:1#") {
    if (led_green_state == LOW)
      toggle_led_green();
    else send_now_data("RES", 11);
  // Open door
  } else if (data == "!1:DOOR:1#") {
    if (door_status == 0) {
      toggle_door();
    } else send_now_data("RES", 21);
  // Close door
  } else if (data == "!1:DOOR:0#") {
    if (door_status == 1) {
      toggle_door();
    } else send_now_data("RES", 20);
  }
}
//----------------------- Init func --------------------------//

void button_led_init() {
  pinMode(LED_RED_PIN, OUTPUT);     // set ESP32 pin to output mode
  pinMode(LED_YELLOW_PIN, OUTPUT);  // set ESP32 pin to output mode
  pinMode(LED_GREEN_PIN, OUTPUT);   // set ESP32 pin to output mode
  button1.setDebounceTime(50);      // set debounce time to 50 milliseconds
  button2.setDebounceTime(50);      // set debounce time to 50 milliseconds
}

void lcd_init() {
  lcd.init();  // initialize LCD
  lcd.backlight();  // turn on LCD backlight
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
}

//----------------------- Other func --------------------------//

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

// Turn on led yellow 5s when button 2 is pressed
void on_led_yellow_5s() {
  if (led_yellow_counter < 500 && led_yellow_state == HIGH) {
    led_yellow_counter += 1;
  } else if (led_yellow_counter == 500) {
    led_yellow_counter = 0;
    led_yellow_state = LOW;
    door_status = !door_status;
    digitalWrite(LED_YELLOW_PIN, led_yellow_state);
    send_now_data("RES", 20 + door_status);
  }
}

void toggle_door() {
  led_yellow_state = HIGH;
  digitalWrite(LED_YELLOW_PIN, led_yellow_state);
}

void toggle_led_green() {
  led_green_state = !led_green_state;
  digitalWrite(LED_GREEN_PIN, led_green_state);
  send_now_data("RES", 10 + led_green_state);
}

void toggle_led_red() {
  led_red_state = !led_red_state;
  digitalWrite(LED_RED_PIN, led_red_state);
}

void toggle_led_red_500ms() {
  if (temp_warning || IR_warning || RS_warning) {
    if (led_red_counter <= 50) {
      led_red_counter += 1;
    } else {
      led_red_counter = 0;
      toggle_led_red();
    }
  } else if ((!temp_warning || !IR_warning || !RS_warning) && led_red_state == HIGH) {
    toggle_led_red();
  }
}

void lcd_print() {
  lcd.clear();
  if (temp_warning) {  // Print Temp warning message
    lcd.setCursor(4, 0);
    lcd.print("Canh bao!");
    lcd.setCursor(2, 1);
    lcd.print("Nhiet do cao");
  } else if (IR_warning) {  // Print IR warning message
    lcd.setCursor(4, 0);
    lcd.print("Canh bao!");
    lcd.setCursor(0, 1);
    lcd.print("Phat hien vatcan");
  } else if (RS_warning) {  // Print RS warning message
    lcd.setCursor(4, 0);
    lcd.print("Canh bao!");
    lcd.setCursor(1, 1);
    lcd.print("Troi dang mua");
  } else {  // Print DHT Temperature & Humidity
    lcd.setCursor(0, 0);
    String do_am = "Do am:    " + String(dht_humi) + "%";
    String nhiet_do = "Nhiet do: " + String(dht_temp);
    lcd.print(do_am);
    lcd.setCursor(0, 1);
    lcd.print(nhiet_do);
    lcd.setCursor(12, 1);
    lcd.print((char)223);
    lcd.setCursor(13, 1);
    lcd.print("C");
  }
}


void dht_process() {
  if (dht_counter <= 200) {
    dht_counter += 1;
  } else {
    dht_counter = 0;
    // Read humidity
    dht_humi = dht.readHumidity();
    // Read temperature as Celsius
    dht_temp = dht.readTemperature();

    // Check if any reads failed
    if (isnan(dht_humi) || isnan(dht_temp)) {
      Serial.println(F("Failed to read from DHT sensor!"));
    }

    // Send warning message
    if (dht_temp >= 35 || dht_temp <= 0) {
      if (!temp_warning) send_now_data("WARN", 5); // Send warning message
      temp_warning = 1;
      lcd_print();
    } else {
      temp_warning = 0;
      lcd_print();
    }
  }
}

void send_dht_data() {
  if (send_dht_data_counter < 1000) {
    send_dht_data_counter += 1;
    if (send_dht_data_counter == 500) {
      send_now_data("HUMI", dht_humi);
    }
  } else {
    send_dht_data_counter = 0;
    send_now_data("TEMP", dht_temp);
  }
}

void button_led_process() {
  button1.loop(); 
  button2.loop();
  if (button1.isPressed()) {
    Serial.println("The button 1 is pressed");
    toggle_led_green();
    send_now_data("LED", led_green_state);
  }
  if (button2.isPressed()) {
    Serial.println("The button 2 is pressed");
    toggle_door();
    send_now_data("DOOR", !door_status);
  }
}

void IR_process() {
  IR_currentState = digitalRead(IR_SENSOR_PIN);

  if (IR_lastState == HIGH && IR_currentState == LOW) {
    Serial.println("The obstacle is detected");
    send_now_data("WARN", 6);
    IR_warning = 1;
    lcd_print();
  } else if (IR_lastState == LOW && IR_currentState == HIGH) {
    Serial.println("The obstacle is cleared");
    IR_warning = 0;
    lcd_print();
  }

  IR_lastState = IR_currentState;
}

void rain_sensor_process() {
  RS_currentState = digitalRead(RS_DO_PIN);
  if (RS_lastState == HIGH && RS_currentState == LOW) {
    Serial.println("The rain is detected");
    send_now_data("WARN", 7);
    RS_warning = 1;
    lcd_print();
  } else if (RS_lastState == LOW && RS_currentState == HIGH) {
    Serial.println("The rain is NOT detected");
    RS_warning = 0;
    lcd_print();
  }
  RS_lastState = RS_currentState;
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

void setup() {
  Serial.begin(9600);  // initialize serial
  pinMode(INIT_PIN, OUTPUT);
  digitalWrite(INIT_PIN, HIGH);
  button_led_init();
  dht.begin();
  lcd_init();

  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(RS_DO_PIN, INPUT);

  WiFi.mode(WIFI_MODE_STA);

  WiFi.mode(WIFI_AP_STA);
  int32_t channel = getWiFiChannel("Relax & Drink");
  Serial.println(channel);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

  esp_now_init_func();

  send_now_data("DOOR", door_status);
  delay(200);
  send_now_data("LED", led_green_state);
  digitalWrite(INIT_PIN, LOW);
}

void loop() {
  button_led_process();
  on_led_yellow_5s();
  toggle_led_red_500ms();
  dht_process();
  send_dht_data();
  IR_process();
  rain_sensor_process();
  delay(10);
}
