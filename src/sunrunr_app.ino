// This #include statement was automatically added by the Particle IDE.
#include <AssetTracker.h>

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_VEML6070.h>

#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
unsigned long lastSync = millis();


//using namespace std;

bool executeStateMachines = false;

Adafruit_VEML6070 UVTracker = Adafruit_VEML6070();
AssetTracker locationTracker = AssetTracker();

float totalUV = 0.0;

// assign SETUP button's pin
int button = BTN;


void setup() {
    
    
    
    Serial.begin(9600); //define the baud rate
    
    
    //particle.syncTime();
    // Initialize the gps and turn it on    
    locationTracker.begin();
    locationTracker.gpsOn();
    
    //Initialize the UV sensor
    UVTracker.begin(VEML6070_1_T);
    
    // Setup pin mode for button
    pinMode(button, INPUT);
    // Setup webhook subscribe
    pinMode(BTN, INPUT_PULLUP);
    
    Particle.subscribe("hook-response/hit", myHandler, MY_DEVICES);
}


void loop() {

      if (millis() - lastSync > ONE_DAY_MILLIS) {
      // Request time synchronization from the Particle Device Cloud
      Particle.syncTime();
      lastSync = millis();
    }

    Serial.print(lastSync);
    String data = "";
    //String apikey = "W29ZALZqQ9Xdgxdd5bzaEM7IEbDTCaqO";
    //String deviceId = "4c001f000e504b464d323520";
    
    //Get the UV values
    totalUV = UVTracker.readUV();
    
    locationTracker.updateGPS(); // get the location
    
    Serial.println("Checking for Fix");
    if (locationTracker.gpsFix()) { //GPS Fixed
        data = String::format("{ \"long\": \"%f\", \"lat\": \"%f\", \"GPS\": \"%f\", \"uv\": \"%f\" }", locationTracker.readLonDeg(), locationTracker.readLatDeg(), locationTracker.getSpeed(), totalUV);  
        Serial.println("Fix"); 
        Particle.publish("hit", data, PRIVATE);
    }
    else {
        Serial.println("NO Fix of course"); //GPS not fixed
        data = String::format("{ \"long\": \"%f\", \"lat\": \"%f\", \"GPS\": \"%f\", \"uv\": \"%f\" }", 999.999, 999.999, 0.0, totalUV);
        }
    Serial.println(data);
    
    if (digitalRead(button) == 0) {
        Serial.println("button pressed!");
        data = String::format("{ \"long\": \"%f\", \"lat\": \"%f\", \"GPS\": \"%f\", \"uv\": \"%f\"}", -111.308585, 32.446482, 5.69, totalUV);
        Serial.println(data);
        // Publish to webhook
        Particle.publish("hit", data, PRIVATE);
    }

    delay(1000);
}


void myHandler(const char *event, const char *data) {
  // Formatting output
  String output = String::format("Response from Post:\n  %s\n", data);
  // Log to serial console
  Serial.println(output);
}