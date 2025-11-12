/*
 * This is the primary config file where you declare all your sensitive variables and definitions in this header
 * Never make this public. This file is an example
 */

#define WIFI_SSID "<wifi_ssid_here>"
#define WIFI_PASSWORD "<wifi_password_here>"
#define THINGNAME "yourThingName"
#define PUB_TOPIC "yourThingName/data"
#define SUB_TOPIC "yourThingName/commands"

const char AWS_IOT_ENDPOINT[] = "your-endpoint-url-goes-here";

// Amazon Root CA 1
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
<Paste-the-AWS-Root-CA-Cert-Content-Here>
-----END CERTIFICATE-----
)EOF";

// Device Certificate
static const char AWS_CERT_CRT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
<Paste-the-Thing-Device-Cert-Content-Here>
-----END CERTIFICATE-----
)EOF";

// Device Private Key
static const char AWS_CERT_PRIVATE[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
<Paste-the-Private-Key-Content-Here>
-----END RSA PRIVATE KEY-----
)EOF";
