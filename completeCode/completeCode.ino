#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <WiFiClient.h>

// WiFi Credentials
const char* WIFI_SSID     = "wifi_network_name";
const char* WIFI_PASSWORD = "wifi_password";

// AWS IoT Core Config
const char* AWS_ENDPOINT  = "endpoint_from_aws_iot_core_console";   // e.g. abcdef123456-ats.iot.us-east-1.amazonaws.com should be visible under domain management in IoT core dash
const int   AWS_PORT      = 8883;
const char* THINGNAME     = "yourNodeName";
const char* PUB_TOPIC     = "yourNodeName/data";  // ensure your policy allows publishing here

/*
 * Certificates: 
 *   These are very important never push these to repo or make public
 *   unless you want some random compromising your IoT network or you want to get billed your savings :D
 *   Paste the exact file contents in the spaces provided
 *   You will need the 3 files from when you create your IoT thing
 *   RootCA Cert, Device Cert, Private Key (You might as well download the public key despite the fact it isnt required!)
*/

// AmazonRootCA1.pem
static const char AWS_ROOT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
------------Paste your Root CA Cert here-----------
-----END CERTIFICATE-----
)EOF";

// *-certificate.pem.crt (AWS thing device certificate)
static const char DEVICE_CERT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
------------Paste your Device Certificate here-----------
-----END CERTIFICATE-----
)KEY";

// *-private.pem.key (AWS thing device private key)
static const char DEVICE_PRIVATE_KEY[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
------------Paste your Private key here-----------
-----END RSA PRIVATE KEY-----
)KEY";

/* 
 * DHT 22 (We are using the GPIO pin D4 so we define the DHTPIN as 4
 * You can use any digital pin for this of course we use this because since the breadboard is not as wide so we expose
 * only the side with I2C so as to accomodate the OLED display and D4 is one of the available ones on that side
 * You define the pin as per the GPIO pin number
 * For example if you are using GPIO18 then pin would be defined as 18 regardless of its a digital or analog or tx/rx pin
 * When in doubt always refer to the pinout diagram and specsheet
 */

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// I2C OLED Config
#define I2C_SDA 21
#define I2C_SCL 22
#define OLED_ADDR 0x3C
#define OLED_RESET -1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// LED Pin
// Again same as DHT config we arent choosing it for any specific reason just using whats available
#define LED_PIN 5

// Networking / MQTT Config
WiFiClientSecure net;
MQTTClient mqtt(256);

// Helper Functions

// This connects us to the Wifi
void connectWiFi() {
  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(400); Serial.print("."); }
  Serial.printf("\nWiFi OK  IP=%s\n", WiFi.localIP().toString().c_str());
}

// For checking connectivity with AWS
// For the various status code please refer to AWS documentation
// PS: They have the tendency to keep changing stuff all the time, so make sure you go through the aws and the library docs
void ensureMqtt() {
  while (!mqtt.connected()) {
    Serial.print("Connecting MQTT to AWS as clientId=");
    Serial.println(THINGNAME);
    if (mqtt.connect(THINGNAME)) {
      Serial.println("MQTT connected");
    } else {
      Serial.printf("MQTT connect failed, status=%d. Retrying...\n", mqtt.lastError());
      delay(1500);
    }
  }
}

// To make sure if we are online and not stuck without internet
// Just a glorified ping command
void testInternet() {
  WiFiClient testClient;
  Serial.print("Testing Internet via google.com ... ");
  if (testClient.connect("google.com", 80)) {
    Serial.println("Online");
    testClient.stop();
  } else {
    Serial.println("No Internet access (can't reach google.com)");
  }
}

// Publishes the data to the topic in AWS
void publishSensor(float t, float h) {
  char payload[128];
  snprintf(payload, sizeof(payload), "{\"temperature\":%.2f,\"humidity\":%.2f}", t, h);
  bool ok = mqtt.publish(PUB_TOPIC, payload);
  Serial.printf("Publish %s: %s\n", ok ? "OK" : "FAIL", payload);
}

/*
 * This is the section for setup
 * We setup our hardware here
 * All the code that needsto be run once and forgotten stays here such as inittialization
 * We should never put any code that needs to be repeatedly called in this setup it should be put in a loop
 * Also it is a very bad practice if you need to initialize something repeatedly it should ideally be initialized once per app
 * And it needs to stay that way unless the hardware config is changed such as switching the GPIO pins for connections, etc
 */
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  dht.begin();

  // I2C init
  Serial.println("Initializing I2C bus...");
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 init failed"));
    while (true) {}
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Booting..."));
  display.display();

  connectWiFi();
  testInternet();

  // TLS credentials for ESP32 (mbedTLS)
  net.setCACert(AWS_ROOT_CA);
  net.setCertificate(DEVICE_CERT);
  net.setPrivateKey(DEVICE_PRIVATE_KEY);
  configTime(0, 0, "pool.ntp.org", "time.nist.gov"); // Required to make sure the time is correct on ESP-32 because if not you will face ton of TLS errors and its a pain to debug trust me

  // Configure MQTT over TLS
  // P.S.. TLS can be annoying to work with at times but its what keeps everything secure and is a major deterent to the Man in the Middle Attacks which hardware is prone to regularly
  mqtt.begin(AWS_ENDPOINT, AWS_PORT, net);

  // Make sure we are connected to AWS
  ensureMqtt();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("AWS MQTT OK"));
  display.display();
  delay(1000);
}

/*
 * We put in all the code that needs to be running constantly
 * So basically all the monitoring and your actions go here
 */
void loop() {
  // keep connections alive
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) ensureMqtt();
  mqtt.loop();

  // Read the data from the sensor
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Print the readout to the OLED display
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0); // Defines where do you print a line for more info we highly encourage you to read the ADAFRUIT SSD1306 docs
  display.println(F("Temp & Humidity"));

  // If for any reason it returns NAN values its usually either sensor got disconnected or rarely the sensor failed
  if (isnan(h) || isnan(t)) {
    display.setCursor(0, 16);
    display.println(F("Sensor Error!"));
    Serial.println(F("DHT read failed"));
  }
  // Write the sensor values to OLED display
  else {
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.printf("%.1fC", t);
    display.setCursor(0, 44);
    display.printf("%.1f%%", h);

    publishSensor(t, h);
  }

  // This displays all the values
  display.display();

  // Blinks the external LED and introduces a 5s delay between each read write cycle
  // delay() takes values in milliseconds
  digitalWrite(LED_PIN, HIGH); 
  delay(300);
  digitalWrite(LED_PIN, LOW);  
  delay(4700); // 5s cycle
}
