#include <WiFi.h>
#include <WiFiClient.h>

/*
 * This program connects the ESP32 to a specified WiFi network
 * and checks Internet connectivity by trying to reach devwmu.com.
 */

/* WiFi credentials */
const char* WIFI_SSID     = "wifi_network_name";
const char* WIFI_PASSWORD = "wifi_password";

/* 
 * Connects to the WiFi network and waits until connected.
 */
void connectWiFi() {
  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.printf("\nConnected to WiFi, IP address: %s\n", WiFi.localIP().toString().c_str());
}

/* 
 * Attempts to open a TCP connection to devwmu.com:80.
 * This serves as a basic reachability test.
 */
void testInternet() {
  WiFiClient client;
  Serial.print("Testing connectivity to devwmu.com ... ");

  if (client.connect("devwmu.com", 80)) {
    Serial.println("Online");
    client.stop();
  } else {
    Serial.println("Failed to connect (Offline)");
  }
}

/* Arduino setup routine */
void setup() {
  Serial.begin(115200);
  connectWiFi();
  testInternet();
}

/* Arduino loop routine */
void loop() {
  /* Repeat connectivity test every 10 seconds */
  testInternet();
  delay(10000);
}
