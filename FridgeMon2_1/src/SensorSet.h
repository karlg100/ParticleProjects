// SensorSet Class
// Here we simply manage the sensors for the cluster
//   - a single DHT22
//   - a single photoresistor
// This class should be called by a timer to maintain the sample, one per cluster

#define PUBLISH_INTERVAL  10000

char VERSION[64] = "0.04";

class SensorSet {
    private:
        // pin for DHT
        int DHTPin = 0;                             // DHT signal pin
        int DHTType = 0;                            // Type of DHT
        int PRPin = 0;                              // Photo Resistor signal pin
        int PRPwrPin = 0;                           // Photo Resistor Power Pin
        bool bDHTstarted = false;		            // flag to indicate we started acquisition
        unsigned long lastPublish = 0;
        PietteTech_DHT* DHT;

    public:
        float tempC = 0, temp = 0, humid = 0, dewPoint = 0, lightLevel = 0;   // our readings
        bool firstSample = false;
        char name[10];

        SensorSet(char nm[10], int pin1, int dhtType, int pin2, int pin3) {
            DHTPin = pin1;
            DHTType = dhtType;
            PRPin = pin2;
            PRPwrPin = pin3;
            strcpy(name, nm);

            // Setup hardware for PhotoResistor
            pinMode(PRPin, INPUT);
            pinMode(PRPwrPin, OUTPUT);
            digitalWrite(PRPwrPin, HIGH);

            // DHT Lib instantiate
            DHT = new PietteTech_DHT(DHTPin, DHTType);
            Particle.publish("DHT22 - firmware version", VERSION, 60, PRIVATE);
            Serial.println("SensorSet() defined");
            delay(1000);

            // for debugging
            //pinMode(D7, OUTPUT);
            //digitalWrite(D7, LOW);
        }

        void sample() {
          char msg[50];
    	    if (!bDHTstarted) {		// start the sample
              //Serial.println("loop() - DHT Sampling");
    	        DHT->acquire();
    	        bDHTstarted = true;
              //digitalWrite(D7, HIGH);
    	    } else {
                if (!DHT->acquiring()) {		// has sample completed?
                    //Serial.println("loop() - DHT sample acquiried");
                    tempC = DHT->getCelsius();
                    temp = DHT->getFahrenheit();
                    humid = DHT->getHumidity();
                    dewPoint = DHT->getDewPoint();
                    lightLevel = round(analogRead(PRPin)*100/4096);
                    bDHTstarted = false;
                    firstSample = true;
                    //controlSensorLEDs(temp, humid, dewPoint);
                    //digitalWrite(D7, LOW);
                    sprintf(msg, "Name: %s :: Temp : %0.2f F", name, temp);
                    Serial.println(msg);
                    sprintf(msg, "Name: %s :: Humidity %0.2f %%", name, humid);
                    Serial.println(msg);
                    sprintf(msg, "Name: %s :: DewPoint %0.2f F", name, dewPoint);
                    Serial.println(msg);
                    sprintf(msg, "Name: %s :: Light Level %0.2f %%", name, lightLevel);
                    Serial.println(msg);
                } else {
                    Serial.println("loop() - DHT no sample");
                }

                //lightSensor = blynkLightLevel(BLK_LIGHT);
    	    }
	    }

      void publishSamples() {
          char msg[50];
          if (millis() > lastPublish) {
            sprintf(msg, "temperature=%0.0f", tempC);
            Particle.publish("temp", msg);
            sprintf(msg, "humidity=%0.0f", humid);
            Particle.publish("humidity", msg);
            sprintf(msg, "light=%0.0f", lightLevel);
            Particle.publish("light", msg);
            lastPublish = millis() + PUBLISH_INTERVAL;
          }
      }

	    void callback() {
	        sample();
          publishSamples();
	    }
};
