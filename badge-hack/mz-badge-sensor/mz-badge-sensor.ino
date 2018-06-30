//==========badge epaper display stuff=====================

// GxEPD lib and display drivers
#include <GxEPD.h>
#include <GxGDEW0213Z16/GxGDEW0213Z16.cpp>  // 2.13" b/w/r
#include GxEPD_BitmapExamples

// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>

// Those are from the board definition and don't need to be defined as they are standard for NINA
//static const uint8_t SS    = 5;  //GPIO28
//static const uint8_t MOSI  = 23; //GPIO1
//static const uint8_t MISO  = 19; // not used for waveshare display
//static const uint8_t SCK   = 18; // GPIO29

// Specific pins used on the MakeZurich badge, adjust if you are using the Display and the NINA standalone
static const uint8_t DC = 22;      //GPIO20
static const uint8_t RST = 21;     //GPIO8
static const uint8_t BUSYN = 4;    //GPIO24

GxIO_Class io(SPI, SS, DC, RST); 
GxEPD_Class display(io, RST, BUSYN); 

//============= Wifi stuff =====================


#include <WiFi.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

const char* ssid     = "impacthub";
const char* password = "coworking@ImpactHub";


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
  handleInterrupButtonDebounce(BTN1, "Button 1 was pushed", count.quick); // Handle BTN1
}

void IRAM_ATTR handleInterruptBTN2() {
  handleInterrupButtonDebounce(BTN2, "Button 2 was pushed", count.mid); // Handle BTN2
}
void IRAM_ATTR handleInterruptBTN3() {
  handleInterrupButtonDebounce(BTN3, "Button 3 was pushed", count.slow); // Handle BTN3
}

void IRAM_ATTR handleInterruptBTN4() {
  uint16_t dummy;
  handleInterrupButtonDebounce(BTN4, "Button 4 was pushed", dummy); // Handle BTN4
}

/**
 *  This is the handler actually doing the work, locking the mutex (one interrupt at a time) and
 * debouncing it so it counts only if the time beetween both interrupts is greater than 100ms.
 * */
void handleInterrupButtonDebounce(byte buttonPin, String text, uint16_t &counter) {
  portENTER_CRITICAL_ISR(&mux);
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 100) {
    // distinguish between pressed and released state
    if(digitalRead(buttonPin) == LOW) {
      Serial.print("[pressed] ");
      counter++;
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
void printMessage()
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

#define HAS_RED_COLOR

void showMZText()
{
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold18pt7b);
  display.setRotation(3);
  display.setCursor(0, 0);
  display.println();
  
  // Print MakeZurich big using red for upper case letters
  display.setTextColor(GxEPD_RED);
  display.print("M");
  display.setTextColor(GxEPD_BLACK);
  display.print("ake");
  display.setTextColor(GxEPD_RED);
  display.print("Z");
  display.setTextColor(GxEPD_BLACK);
  display.print("urich");
  display.println();

  // Print the event date small
  display.setFont(&FreeMonoBold9pt7b);
  display.println("Juni 22-30, 2018");
  display.update();
  delay(10000);
  display.setRotation(0);
}

void showDataSendt(String &jsonString)
{
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold18pt7b);
  display.setRotation(3);
  display.setCursor(0, 0);
  display.println();
  
  // Print MakeZurich big using red for upper case letters
  display.setTextColor(GxEPD_RED);
  display.print("D");
  display.setTextColor(GxEPD_BLACK);
  display.print("ata");
  display.setTextColor(GxEPD_RED);
  display.print("S");
  display.setTextColor(GxEPD_BLACK);
  display.print("endt:");
  display.println();

  display.setFont(&FreeMonoBold9pt7b);
  display.println("random");
  display.update();
  delay(10000);
  display.setRotation(0);
}

void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    Serial.begin(115200);
    //display.init(115200); // enable diagnostic output on Serial
    delay(1000);
    //showMZText();

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

    noInterrupts(); 

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    interrupts();

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
    printMessage();

    //https://gateway.hivemind.ch/v1/capture/3157b1a0419836bc807b11274001553c?id=PRAKTIKANT1

    // We now create a URI for the request
    const char* host = "gateway.hivemind.ch";
    String url = "/v1/capture/3157b1a0419836bc807b11274001553c?id=PRAKTIKANT1";
    String jsonData = "{\"hikers\":";
    jsonData += count.slow;
    jsonData += ",\"bikers\":";
    jsonData += count.mid;
    jsonData += ",\"horses\":";
    jsonData += count.quick;
    jsonData += "}";

    Serial.print("connecting to ");
    Serial.println(host);

    noInterrupts(); 

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(host, httpPort)) {
        Serial.println("connection failed");
        return;
    }

    Serial.print("Requesting URL: ");
    Serial.println(url);
    String postData = String("POST ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" + 
                 "Content-Type: " + "application/json\r\n"
                 "Content-Length: " + jsonData.length() + "\r\n\r\n" +
                 jsonData + "\r\n" ;
                 //+ "Connection: close\r\n\r\n";
    Serial.println("POST data: ");
    Serial.print(postData);

    // This will send the request to the server
    client.print(postData);
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 10000) {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
        }
    }

    interrupts(); 

    // Read all the lines of the reply from server and print them to Serial
    while(client.available()) {
        String line = client.readStringUntil('\r');
        Serial.print(line);
    }
    

    Serial.println();
    Serial.println("closing connection");
    //showDataSendt(jsonData);
    resetCounts(count);
}

