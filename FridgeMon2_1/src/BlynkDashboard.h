
#include <blynk.h>

// blynk dashboard
// Blynk output pins
#define BLK_TEMP V1
#define BLK_HUMIDITY V2
#define BLK_DEWPOINT V3
#define BLK_LIGHT V4

#define BLK_DOOR_LAST_OPENED V50
#define BLK_DOOR_LAST_CLOSED V51

#define BLK_DOOR_LED V10
#define BLK_TEMP_LED V11
#define BLK_HUMIDITY_LED V12
#define BLK_FROST_LED V13

#define BLK_TERMINAL V0

// Blynk Virtual input pins
#define BLK_TEMP_ALM_LOW V99
#define BLK_TEMP_ALM_HIGH V100

// chirp durations
#define LONG_CHIRP 100
#define SHORT_CHIRP 50
#define CHIRP_DELAY 50
#define CHIRP_SLEEP 10000

#define DOOR_OPEN_LEVEL 50              // what % of light level is considered the door to be open
#define BLK_DOOR_ALM V102

#define BLK_DEBUG_LEVEL V127

WidgetTerminal terminal(BLK_TERMINAL);

class BlynkDashboard {
    private:
        char deviceName[50];
        char msg[50];
        char tempInChar[10];
        char auth[40];
        bool isFirstConnect = true;
        unsigned long doorOpen = 0;                   // dtection flag for the door open
        unsigned long blkLastAlarm = 0;
        unsigned long blkNextAlarm = 0;
        unsigned long lastChirp = 0;
        unsigned long lastNotice = 0;
        int blinkCounter = 0;
        SensorSet* Sensor;
        //WidgetTerminal* terminal;
        //BlynkParticle* Blynk;

        void blynkHumid(int pin) {
            float humid = Sensor->humid;
            sprintf(tempInChar,"%0.1f", humid);
            sprintf(msg, "Humidity %0.2f %%", humid);
            if (debugLevel >= 2) terminal.println(msg);
            Blynk.virtualWrite(pin, tempInChar);
        }

        void blynkTemp(int pin) {
            sprintf(tempInChar,"%0.1f", Sensor->temp);
            Blynk.virtualWrite(pin, tempInChar);
            sprintf(msg, "Temperature %0.2f F", Sensor->temp);
            if (debugLevel >= 2) terminal.println(msg);
        }

        void blynkDewPoint(int pin) {
            sprintf(tempInChar,"%0.1f", Sensor->dewPoint);
            Blynk.virtualWrite(pin, tempInChar);
            sprintf(msg, "Dew Point %0.2f F", Sensor->dewPoint);
            if (debugLevel >= 2) terminal.println(msg);
        }

        void blynkLightLevel(int pin) {
            Blynk.virtualWrite(pin, Sensor->lightLevel);
            sprintf(msg, "Light Level %0.2f %%", Sensor->lightLevel);
            if (debugLevel >= 2) terminal.println(msg);
        }
        void checkDoor() {
            if (Sensor->lightLevel > DOOR_OPEN_LEVEL) {
                if (doorOpen == 0) {
                    strncpy(msg, Time.timeStr(), sizeof(Time.timeStr()-1));
                    sprintf(msg, "Door Opened %s", msg);
                    if (debugLevel >= 2) terminal.println(msg);
                    Blynk.virtualWrite(BLK_DOOR_LAST_OPENED, Time.format(Time.now(), TIME_FORMAT_DEFAULT));
                    doorOpen = millis();
                }
            } else if (doorOpen > 0) {
                strncpy(msg, Time.timeStr(), sizeof(Time.timeStr()-1));
                sprintf(msg, "Door Closed %s", msg);
            if (debugLevel >= 2) terminal.println(msg);
                Blynk.virtualWrite(BLK_DOOR_LAST_CLOSED, Time.format(Time.now(), TIME_FORMAT_DEFAULT));
                doorOpen = 0;
            }
        }

        bool blynkNotify(char msg[255]) {
            char msg2[255], msg3[255];
            sprintf(msg3, "%s : %s", deviceName, msg);
            if (Blynk.connected()) {
                if (millis() < blkNextAlarm) {
                    sprintf(msg2, "blynkNotify() - waitng for next time window %d < %d", millis(), blkNextAlarm);
                    if (debugLevel >= 1) terminal.println(msg2);
                } else {
                    Blynk.notify(msg3);
                    sprintf(msg2, "blynkNotify() - notification sent");
                    if (debugLevel >= 1) terminal.println(msg2);
                }
                return true;
            } else {
                return false;
            }
        }

        void doChirp(int cycles) {
            if (millis() < lastChirp)
                return;
            sprintf(msg, "doChirp(%d) - %d", cycles, lastChirp);
            if (debugLevel >= 1) terminal.println(msg);
            digitalWrite(SPEAKER, HIGH);
            delay(LONG_CHIRP);
            digitalWrite(SPEAKER, LOW);
            for (int i=0; i<cycles; i++) {
                delay(CHIRP_DELAY);
                digitalWrite(SPEAKER, HIGH);
                delay(SHORT_CHIRP);
                digitalWrite(SPEAKER, LOW);
                delay(LONG_CHIRP);
            }
        }

        void checkAlarms() {
            bool alarmState = false;
            //sprintf(msg, "checkAlarms() - start - %d %d", millis(), blkNextAlarm);
            //if (debugLevel >= 4) terminal.println(msg);
            sprintf(msg, "checkAlarms() - alarm check time");
            if (debugLevel >= 3) terminal.println(msg);
            if (doorOpen > 0 && (millis()-doorOpen)/1000 > (float)doorAlarmThresh) {
                sprintf(msg, "Door has been open for %d seconds", round((millis()-doorOpen)/1000));
                if (debugLevel >= 1) terminal.println(msg);
                alarmState = true;
                blynkNotify(msg);
                doChirp(1);
            }
            if (Sensor->temp <= (float)tempAlarmThreshLow && round(Sensor->temp) != -3) {
                sprintf(msg, "Temperature is too low! %0.2f F < %d F", Sensor->temp, tempAlarmThreshLow);
                if (debugLevel >= 1) terminal.println(msg);
                alarmState = true;
                blynkNotify(msg);
                doChirp(2);
            }
            if (Sensor->temp >= (float)tempAlarmThreshHigh && round(Sensor->temp) != -3) {
                sprintf(msg, "Temperature is too high! %0.2f F > %d F", Sensor->temp, tempAlarmThreshHigh);
                if (debugLevel >= 1) terminal.println(msg);
                alarmState = true;
                blynkNotify(msg);
                doChirp(3);
            }
            if (alarmState == true) {
                if (blkNextAlarm < millis()) {
                    blkLastAlarm = millis();
                    blkNextAlarm = millis()+1000*60*30;
                }
                if (millis() > lastChirp) lastChirp = millis() + CHIRP_SLEEP;
                sprintf(msg, "checkAlarms() - alarm state true, vars %d %d", blkLastAlarm, blkNextAlarm);
                if (debugLevel >= 1) terminal.println(msg);
            } else {
//                blkLastAlarm = 0;
//                blkNextAlarm = 0;
                if (debugLevel >= 1) terminal.println("checkAlarms() - no alarm states or blynk not connected");
            }
        }

        void controlDoorLED() {
            if (Sensor->lightLevel > DOOR_OPEN_LEVEL)
                Blynk.virtualWrite(BLK_DOOR_LED, 1023);
            else
                Blynk.virtualWrite(BLK_DOOR_LED, 0);
        }

        void controlSensorLEDs() {
            if (Sensor->temp >= tempAlarmThreshHigh || Sensor->temp <= tempAlarmThreshLow)
                Blynk.virtualWrite(BLK_TEMP_LED, 1023);
            else
                Blynk.virtualWrite(BLK_TEMP_LED, 0);

            if (Sensor->humid >= humidAlarmThreshHigh)
                Blynk.virtualWrite(BLK_HUMIDITY_LED, 1023);
            else
                Blynk.virtualWrite(BLK_HUMIDITY_LED, 0);

            if (Sensor->temp <= Sensor->dewPoint)
                Blynk.virtualWrite(BLK_FROST_LED, 1023);
            else
                Blynk.virtualWrite(BLK_FROST_LED, 0);
        }

    public:
      int tempAlarmThreshLow = 9999;      // alarm threshold from blynk
      int tempAlarmThreshHigh = 9999;     // alarm threshold from blynk
      int doorAlarmThresh = 0;            // alarm threshold from blynk
      int humidAlarmThreshHigh = 90;      // alarm threshold from blynk
      int debugLevel = 0;                 // debug level for the terminal output

        BlynkDashboard(String name, SensorSet* s1, String authStr) {
            authStr.toCharArray(auth, 40);
            sprintf(msg, "New Blynk Dashboard declaired");
            Serial.println(msg);
            // reset alarm times
            blkLastAlarm = 0;
            blkNextAlarm = 0;
            lastChirp = 0;
            name.toCharArray(deviceName, 40);
            Sensor = s1;

            Serial.println(authStr);
            Serial.println(auth);
            Blynk.begin(auth);

            terminal.println("blynk started up");
            terminal.flush();
        }

        void updateDash() {
          //Blynk.virtualWrite(V50, Time.format(Time.now(), TIME_FORMAT_DEFAULT));
          //Blynk.virtualWrite(V51, Time.format(Time.now(), TIME_FORMAT_DEFAULT));
          if (Sensor->firstSample == false) {
            if (debugLevel >= 4) {
              sprintf(msg,"waiting on first sample");
              terminal.println(msg);
            }
          } else {
            checkDoor();
            controlDoorLED();
            if (debugLevel >= 4) {
              sprintf(msg,"doorOpen %d - %d seconds", doorOpen, round((millis()-doorOpen)/1000));
              terminal.println(msg);
              sprintf(msg,"tempAlarmThreshHigh %d F", tempAlarmThreshHigh);
              terminal.println(msg);
              sprintf(msg,"tempAlarmThreshLow %d F", tempAlarmThreshLow);
              terminal.println(msg);
              sprintf(msg,"doorAlarmThresh %d seconds", doorAlarmThresh);
              terminal.println(msg);
            }

            blynkTemp(BLK_TEMP);
            blynkHumid(BLK_HUMIDITY);
            blynkDewPoint(BLK_DEWPOINT);
            controlSensorLEDs();
            checkAlarms();
            blynkLightLevel(BLK_LIGHT);
          }
        }

        void blynkService() {
            Blynk.run();
            if (tempAlarmThreshLow == 9999)
              Blynk.syncAll();
            if (debugLevel >= 3) {
              sprintf(msg, "Name: %s :: Temp : %0.2f F", Sensor->name, Sensor->temp);
              terminal.println(msg);
//              Serial.println(msg);
              sprintf(msg, "Name: %s :: Humidity %0.2f %%", Sensor->name, Sensor->humid);
              terminal.println(msg);
//              Serial.println(msg);
              sprintf(msg, "Name: %s :: DewPoint %0.2f F", Sensor->name, Sensor->dewPoint);
              terminal.println(msg);
//              Serial.println(msg);
              sprintf(msg, "Name: %s :: Light Level %0.2f %%", Sensor->name, Sensor->lightLevel);
              terminal.println(msg);
//              Serial.println(msg);
              terminal.flush();
            }
          updateDash();
        }
};
