#include <TheThingsNetwork.h>

// Set your AppEUI and AppKey
const char *appEui = "70B3D57ED00101BB";
const char *appKey = "DC9D402F303AE61DBE15EB474F1481B4";

#define loraSerial Serial1
#define debugSerial Serial

// Replace REPLACE_ME with TTN_FP_EU868 or TTN_FP_US915
#define freqPlan TTN_FP_EU868

TheThingsNetwork ttn(loraSerial, debugSerial, freqPlan);

void setup()
{
  loraSerial.begin(57600);
  debugSerial.begin(9600);

  // Wait a maximum of 10s for Serial Monitor
  while (!debugSerial && millis() < 10000);

  //============= do your sensor setup here ================


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

  // Prepare payload of 1 byte to indicate LED status
  byte payload[1];
  payload[0] = (digitalRead(LED_BUILTIN) == HIGH) ? 1 : 0;
  // Send it off
  ttn.sendBytes(payload, sizeof(payload));

  delay(10000);
}

void message(const byte *payload, size_t length, port_t port)
{
  debugSerial.println("-- MESSAGE");

  // Only handle messages of a single byte
  if (length != 1)
  {
    return;
  }

  if (payload[0] == 0)
  {
    debugSerial.println("LED: off");
    digitalWrite(LED_BUILTIN, LOW);
  }
  else if (payload[0] == 1)
  {
    debugSerial.println("LED: on");
    digitalWrite(LED_BUILTIN, HIGH);
  }
}
