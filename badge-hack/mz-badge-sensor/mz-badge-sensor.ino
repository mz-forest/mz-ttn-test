/*
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *
 */

#include <WiFi.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

const char* ssid     = "impacthub";
const char* password = "coworking@ImpactHub";
const char* host = "data.sparkfun.com";

typedef struct 
{
  uint16_t slow;
  uint16_t mid;
  uint16_t quick;
} measurements;

measurements count;


// PINS
// Buttons
const byte BTN1 = D19;
const uint64_t BTN1_mask = 1ULL << BTN1;
const byte BTN2 = D8;
const uint64_t BTN2_mask = 1ULL << BTN2;
const byte BTN3 = D21;
const uint64_t BTN3_mask = 1ULL << BTN3;
const byte BTN4 = D20;
const uint64_t BTN4_mask = 1ULL << BTN4;

// interrupts
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR handleInterruptBTN1() { 
  handleInterrupButtonDebounce(BTN1, "Button 1 was pushed"); // Handle BTN1
  count.quick++;
}

void IRAM_ATTR handleInterruptBTN2() {
  handleInterrupButtonDebounce(BTN2, "Button 2 was pushed"); // Handle BTN2
  count.mid++;
}
void IRAM_ATTR handleInterruptBTN3() {
  handleInterrupButtonDebounce(BTN3, "Button 3 was pushed"); // Handle BTN3
  count.slow++;
}

void IRAM_ATTR handleInterruptBTN4() {
  handleInterrupButtonDebounce(BTN4, "Button 4 was pushed"); // Handle BTN4
}

/**
 *  This is the handler actually doing the work, locking the mutex (one interrupt at a time) and
 * debouncing it so it counts only if the time beetween both interrupts is greater than 100ms.
 * */
void handleInterrupButtonDebounce(byte buttonPin, String text) {
  portENTER_CRITICAL_ISR(&mux);
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 100) {
    // distinguish between pressed and released state
    if(digitalRead(buttonPin) == LOW) {
      Serial.print("[pressed] ");
    } else {
      Serial.print("[released] ");
    }
    Serial.println(text);
    last_interrupt_time = interrupt_time;
  }
  portEXIT_CRITICAL_ISR(&mux);
  // Configure wakeup from deep sleep on all 4 buttons
}

//============= do your TX/RX functions here ================
void resetCounts(measurements &c)
{
  c.slow = 0;
  c.mid = 0;
  c.quick = 0;
}

//transmit counts to ttn
void sendMessage()
{
  Serial.println();
  Serial.print("Slow: ");
  Serial.print(count.slow);
  Serial.print(" Mid: ");
  Serial.print(count.mid);
  Serial.print(" Quick: ");
  Serial.println(count.quick);
  Serial.println();
}

void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    Serial.begin(115200);
    delay(10);

    // Setup interrupts for Buttons
    pinMode(BTN1, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BTN1), handleInterruptBTN1, FALLING);
    pinMode(BTN2, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BTN2), handleInterruptBTN2, FALLING);
    pinMode(BTN3, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BTN3), handleInterruptBTN3, FALLING);
    pinMode(BTN4, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BTN4), handleInterruptBTN4, FALLING);
    Serial.println("Initialized interrupts");

    // We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

int value = 0;

void loop()
{
    delay(5000);
    ++value;
    sendMessage();
    resetCounts(count);

    Serial.print("connecting to ");
    Serial.println(host);

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(host, httpPort)) {
        Serial.println("connection failed");
        return;
    }

    // We now create a URI for the request
    String url = "/input/";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
        }
    }

    // Read all the lines of the reply from server and print them to Serial
    while(client.available()) {
        String line = client.readStringUntil('\r');
        Serial.print(line);
    }

    Serial.println();
    Serial.println("closing connection");
}

