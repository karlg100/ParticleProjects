/*
 * Project FridgeMon2_1
 * Description: Fridge remote monitor system based on Particle
 * Author: Karl Grindley
 * Date: 2018-09-30
 */

//#define BLYNK_DEBUG // Optional, this enables lots of prints
//#define BLYNK_PRINT Serial

ApplicationWatchdog wd(60000, System.reset);


// Fridge
#define DHTPIN D4         	        // Digital pin for communications
#define PHR A0
#define PHRPWR A1
#define DHTTYPE  DHT22              // Sensor type DHT11/21/22/AM2301/AM2302
#define SPEAKER D0

#define UPDATE_INTERVAL   1000      // How often to update the dashboard
#define SAMPLE_INTERVAL   100       // How often to pull data from the sensors
#define NOTIFICATION_INTERVAL   10000   // How often should we send a push notification/Publish event?

// globals
char msg[50];
String auth = "auth-unset";
String deviceName = "unset";
unsigned long lastChirp = 0;
unsigned long lastUpdate = 0;
unsigned long lastSample = 0;

#include <PietteTech_DHT.h>
#include "SensorSet.h"
#include "BlynkDashboard.h"

STARTUP(
    WiFi.selectAntenna(ANT_AUTO)   // Auto select antenna for best coverage
    );


bool isFirstConnect = TRUE;
BLYNK_CONNECTED() // runs every time Blynk connection is established
{
    digitalWrite(D7, LOW);
    if (isFirstConnect) {
        // Request server to re-send latest values for all pins
        Blynk.syncAll();
        isFirstConnect = FALSE;
    }
}

BLYNK_DISCONNECTED() // runs every time Blynk connection is established
{
    digitalWrite(D7, HIGH);
}

// Name of device.  Will be populated later once connected to the cloud
void handler(const char *topic, const char *data) {
    Serial.println("received " + String(topic) + ": " + String(data));
    deviceName = String(data);
}

// set the blynk auth string based on the device name
void getBlynkID() {
    //strcpy(sysID, System.deviceID());
//    if (strcmp(deviceName, "basement_fridge") != 0)                                 // hunter_dentist
    if (deviceName == "basement_fridge_top")
        auth = "5ef3e4b0994f413687c34c3777d94fa7";
    else if (deviceName == "basement_fridge_bottom")
        auth = "9a5f7f9576ea40e2931346ef204aa33d";
    else if (deviceName == "basement_freezer")
        auth = "b799f1e0d0b747a881a671f331e3da39";
    else {
      Particle.publish("debug", "unknown device - update getBlynkID() with new device name");
      auth = "unknown";
      return;
    }
    Particle.publish("debug", "blynk authID set");
}

// define the sensor cluster objet
SensorSet* sensorCluster1;
BlynkDashboard* blynkDash1;

BLYNK_WRITE(V99)
{
    blynkDash1->tempAlarmThreshLow = param.asInt();
    sprintf(msg, "Temp Alarm Low changed to %d", blynkDash1->tempAlarmThreshLow);
    Serial.print(msg);
    terminal.println(msg);
    terminal.flush();
}

// High Temp Alarm Threshold
BLYNK_WRITE(V100)
{
    blynkDash1->tempAlarmThreshHigh = param.asInt();
    sprintf(msg, "Temp Alarm High changed to %d", blynkDash1->tempAlarmThreshHigh);
    Serial.print(msg);
    terminal.println(msg);
    terminal.flush();
}

// Door Alarm Threshold
BLYNK_WRITE(V102)
{
    blynkDash1->doorAlarmThresh = param.asInt();
    sprintf(msg, "Door Alarm changed to %d", blynkDash1->doorAlarmThresh);
    Serial.print(msg);
    terminal.println(msg);
    terminal.flush();
}

// reboot button
BLYNK_WRITE(V126)
{
    if (param.asInt() == 1) {
        sprintf(msg, "Reboot command received");
        Serial.print(msg);
        terminal.println(msg);
        terminal.flush();
        System.reset();
    }
}

// safe mode button
BLYNK_WRITE(V125)
{
    if (param.asInt() == 1) {
        sprintf(msg, "Switching to safe mode");
        Serial.print(msg);
        terminal.println(msg);
        terminal.flush();
        System.enterSafeMode();
    }
}

// Debug level slider
BLYNK_WRITE(V127)
{
    blynkDash1->debugLevel = param.asInt();
    sprintf(msg, "Debug Level changed to %d", blynkDash1->debugLevel);
    Serial.print(msg);
    terminal.println(msg);
    terminal.flush();
}

void setup()
{
    Time.zone(-4);                   // Eastern Time Zone
    pinMode(D7, OUTPUT);
    digitalWrite(D7, HIGH);

    // audio alarm
    pinMode(SPEAKER, OUTPUT);

    Serial.begin(9600);

    // get the device name
    Particle.subscribe("particle/device/name", handler);
    Particle.publish("particle/device/name");

    sensorCluster1 = new SensorSet("sensor1", DHTPIN, DHTTYPE, PHR, PHRPWR);

    Serial.println("setup() - complete");
    Particle.publish("debug", "Setup() Complete");
}

bool initComplete = false;

void loop() {

  // device name has acquired, but blynk unconfigured
  if (initComplete) {
    if (millis() > lastSample) {
      sensorCluster1->callback();
      lastSample = millis() + SAMPLE_INTERVAL;
    }
    if (millis() > lastUpdate) {
      blynkDash1->blynkService();
      lastUpdate = millis() + UPDATE_INTERVAL;
    }
  } else if (deviceName != "unset" && auth == "auth-unset") {
    Particle.publish("debug_handler", deviceName, 60, PRIVATE);
    getBlynkID();
  } else if ( auth != "auth-unset") {
    Particle.publish("debug_blynk", auth, 60, PRIVATE);
    blynkDash1 = new BlynkDashboard(deviceName, sensorCluster1, auth);
    initComplete = true;
  }

  wd.checkin(); // resets the AWDT count

  // reboot every 24 hours
  //if (millis() > 86400000)
  //  System.reset();
}
