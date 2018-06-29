#include <TheThingsNetwork.h>

// Set your AppEUI and AppKey
const char *appEui = "70B3D57ED00101BB";
const char *appKey = "E2F258A856665EEBBBDD360D183D2DB7";

const int SENSOR_INT_PIN = 3; // sensor interrupt pin

#define loraSerial Serial1
#define debugSerial Serial

// Replace REPLACE_ME with TTN_FP_EU868 or TTN_FP_US915
#define freqPlan TTN_FP_EU868

typedef struct 
{
  uint16_t slow;
  uint16_t mid;
  uint16_t quick;
} measurements;

measurements count;
uint32_t refresh_ms = 60000;
TheThingsNetwork ttn(loraSerial, debugSerial, freqPlan);

//============= do your sensor functions here ================

void sensorIRQ() {
  count.slow++;
  debugSerial.println("-- IRQ");
}

//============= End of do your sensor functions here =========



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
  //let's fill the data endianness-agnostic
  byte payload[8];
  payload[0] = count.slow / 256;
  payload[1] = count.slow % 256;
  payload[2] = count.mid / 256;
  payload[3] = count.mid % 256;
  payload[4] = count.quick / 256;
  payload[5] = count.quick % 256;

  payload[6] = refresh_ms / 1000 / 256;
  payload[7] = (refresh_ms / 1000) % 256;
  resetCounts(count);
  // Send it off
  ttn.sendBytes(payload, sizeof(payload));
}

//receiving ttn message
void message(const byte *payload, size_t length, port_t port)
{
  debugSerial.println("-- MESSAGE");
  debugSerial.print(length);
  debugSerial.println(" bytes received.");

  // Only handle messages of a single byte
  if (length != 1)
  {
    return;
  }
}

//============= End of TX/RX functions ================

void setup()
{
  loraSerial.begin(57600);
  debugSerial.begin(9600);

  // Wait a maximum of 10s for Serial Monitor
  while (!debugSerial && millis() < 10000);

  resetCounts(count);
  //============= do your sensor setup here ================
  attachInterrupt(digitalPinToInterrupt(SENSOR_INT_PIN), sensorIRQ, RISING);

  //============= End of do your sensor setup here =========

  // Set callback for incoming messages
  ttn.onMessage(message);

  debugSerial.println("-- STATUS");
  ttn.showStatus();

  debugSerial.println("-- JOIN");
  ttn.join(appEui, appKey);
}


void loop()
{
  debugSerial.println("-- LOOP");

  //============= do your sensor work here ================


  //============= End of do your sensor work here =========

  sendMessage();
  delay(refresh_ms);
}


