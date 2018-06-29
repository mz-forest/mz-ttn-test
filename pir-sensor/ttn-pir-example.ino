#include <TheThingsNetwork.h>

//devEui: 0004A30B001C6B1C
const char *appEui = "70B3D57ED00101BB";
const char *appKey = "BB6B2D5A2F2377DF1C92AE3810B96051";

const char *devAddr = "26011B48";
const char *nwkSKey = "0E8A0003A3B267D990194261EE11B7C1";
const char *appSKey = "DCB90E5DFCA1A1FAA42A2AF0A1FA759C";

#define loraSerial Serial1
#define debugSerial Serial

TheThingsNetwork ttn(loraSerial, debugSerial, TTN_FP_EU868);

long int timer_period_to_tx = 60000;
long int timer_millis_lora_tx = 0;
long int delta_lora_tx = 60001;

uint16_t counter = 0;
uint16_t minutes = timer_period_to_tx / 60000;
int motionCount = 0;
const int PIR_MOTION_SENSOR = 6; 
const int RED_LED = 7; 

void setup()
{
  loraSerial.begin(57600);
  debugSerial.begin(9600);

  while (!debugSerial && millis() < 10000);

 //============= do your sensor setup here ================
  pinMode(RED_LED, OUTPUT);
  pinMode(PIR_MOTION_SENSOR, INPUT);
  //============= End of do your sensor setup here =========

  // Set callback for incoming messages
  ttn.onMessage(message);

  debugSerial.println("-- STATUS");
  ttn.showStatus();

  debugSerial.println("-- JOIN");
  //OTAA
  ttn.join(appEui, appKey);

  //ABP
  //ttn.personalize(devAddr, nwkSKey, appSKey);
}

void loop()
{  
  digitalWrite(RED_LED, LOW);
  
  if(digitalRead(PIR_MOTION_SENSOR)) {
    counter++;
    digitalWrite(RED_LED, HIGH);
    delay(5000);
  }
  
  if (delta_lora_tx > timer_period_to_tx) {
    byte payload[8];

    payload[0] = lowByte(counter);
    payload[1] = highByte(counter);

    payload[2] = lowByte(counter);
    payload[3] = highByte(counter);

    payload[4] = lowByte(counter);
    payload[5] = highByte(counter);

    payload[6] = lowByte(minutes);
    payload[7] = highByte(minutes);
    
    debugSerial.print("Transmitting number of detected people: ");
    debugSerial.println(counter);
  
    ttn.sendBytes(payload, sizeof(payload), 2, true);
  
    timer_millis_lora_tx = millis();

    counter = 0;

    delay(5000);
  }

  delta_lora_tx = millis() - timer_millis_lora_tx;
  
  delay(200);
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
