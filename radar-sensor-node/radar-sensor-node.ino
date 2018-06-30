// radar-sensor-node.ino

#include <TheThingsNetwork.h>
#include "src/RadarSensor.h"

// Set your AppEUI and AppKey
const char *appEui = "70B3D57ED00101BB";
const char *appKey = "EFBB096307EBD105F4E52E1ADE2B2DC9";

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

//============= do sensor stuff here ================ 
void getMeasurements() {
  // get sensor data
  uint16_t sensorBins[] = {0, 0, 0};
  readRadarSensor(sensorBins, 3);
  count.slow = sensorBins[2];
  count.mid = sensorBins[1];
  count.quick = sensorBins[0];
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
  debugSerial.println(" bytes received:");

  if (length > 0)
  {
    uint32_t newRefresh = 0;
    for(int i = 0; i < length; i++)
    {
      debugSerial.print(payload[i]);
      newRefresh += payload[i] * pow(256, length - i - 1);
    }
    debugSerial.println();
    if(newRefresh < 5) // todo: check if bigger then max(uint32_t / 1000)
    {
      debugSerial.print("Error, Refresh rate has to be bigger then 5s, but it's ");
      debugSerial.println(newRefresh);
      return;
    }
    else if(newRefresh * 1000 > UINT32_MAX)
    {
      debugSerial.print("Error, Refresh rate has to be smaller then UINT32_MAX, but it's ");
      debugSerial.println(newRefresh);
      return;
    }
    refresh_ms = newRefresh * 1000;
    debugSerial.print("New Refresh ratin in ms: ");
    debugSerial.println(refresh_ms);
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
  setupRadarSensor();
  delay(10000);
  enableRadarSensor();
  
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
  getMeasurements();
  sendMessage();
  printRadarSensor();
  delay(refresh_ms);
}


