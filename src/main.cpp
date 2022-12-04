#include <SoftwareSerial.h>
#include <NMEAGPS.h>
#include <Wire.h>
#include <SH1106Wire.h>
#include <SPI.h>
#include <SD.h>
#include <LinkedList.h>

bool VALID_FIX_ONLY_WITH_HEADING = false;
bool ENABLE_SERIAL_DEBUGGING     = true;
int BUTTON_PIN = D0;

struct GpsFormat {  
  String DateTime;
  String COG;
  String SOG;
  String Latitude;
  String Longitude;
};

void DEBUG(String message) {
  if (!ENABLE_SERIAL_DEBUGGING) { return; }

  Serial.println(message);	
}

class DisplayStateMonitor {
  bool DisplayOn = true;
  unsigned long MinimumSwitchStatePeriod = 500;
  unsigned long LastStateChange = 0;

  public:
    DisplayStateMonitor() { }

    void SetState(bool change, SH1106Wire &display) {
      unsigned long currentMillis = millis();
      bool allowedToChange = ((currentMillis - LastStateChange) > MinimumSwitchStatePeriod);

      if (!change || !allowedToChange) { return; }

      DisplayOn = !DisplayOn;

      if (DisplayOn) {
        display.displayOn();        
      } else {
        display.displayOff();        
      }

      LastStateChange = currentMillis;      
    }
};

class MotionStateMonitor {
  unsigned long ValidHeadingRequiredPeriod = 10 * 1000;
  bool InMovingState = false;
  unsigned long FirstValidHeading = 0;

  public:
    MotionStateMonitor() {}

    void LogHeadingStatus(bool headingStatus) {
      unsigned long currentMillis = millis();

      if (headingStatus && !InMovingState) {
        if (FirstValidHeading == 0) {
          FirstValidHeading = currentMillis;
        }        

        if ((currentMillis - FirstValidHeading) > ValidHeadingRequiredPeriod) {
          InMovingState = true;
        }
      }

      if (!headingStatus && InMovingState) {
        InMovingState = false;
        FirstValidHeading = 0;
      }
    }

    bool IsInMovingState() {
      return InMovingState;
    }
};

class Timings {
  const unsigned long DisplayInterval    =  2 * 1000;
  const unsigned long LoggingInterval    =  5 * 1000;
  const unsigned long CardUpdateInterval = 60 * 1000;

  unsigned long LastDisplay = 0;
  unsigned long LastLog = 0;
  unsigned long LastPosition = 0;
  unsigned long LastCardUpdate = 0;

public:
  Timings() { }

  bool ShouldDisplay() {
    unsigned long currentMillis = millis();

    if ((currentMillis - LastDisplay) > DisplayInterval) {
      LastDisplay = currentMillis;
      return true;
    }

    return false;
  }

  bool ShouldLog() {
    unsigned long currentMillis = millis();        

    if ((currentMillis - LastLog) > LoggingInterval) {
      LastLog = currentMillis;
      return true;
    }

    return false;
  }

  bool ShouldUpdateCard() {
    unsigned long currentMillis = millis();

    if ((currentMillis - LastCardUpdate) > CardUpdateInterval) {
      LastCardUpdate = currentMillis;
      return true;
    }

    return false;
  }  
};

SH1106Wire Display(0x3c, SDA, SCL);
NMEAGPS GPS;
SoftwareSerial SerialGPS(D3, D4);
Timings ActionTimings = Timings();
LinkedList<gps_fix> GPSDataList = LinkedList<gps_fix>();
MotionStateMonitor MotionMonitor = MotionStateMonitor();
DisplayStateMonitor DisplayMonitor = DisplayStateMonitor();

void setup() {
  if (ENABLE_SERIAL_DEBUGGING) {
    Serial.begin(9600);
    while(!Serial) delay(2000);
    DEBUG("Serial working");
  }  

  while(!Display.init()) delay(2000);
  DEBUG("Display working");

  SerialGPS.begin(9600);  
  while(!SerialGPS) delay(2000);
  DEBUG("GPS working");
  
  while(!SD.begin(D8)) delay(2000);  
  DEBUG("SD working");

  Display.flipScreenVertically();  
  Display.clear();  
  Display.display();

  pinMode(BUTTON_PIN, INPUT_PULLDOWN_16);
}

void GPSdebug(gps_fix &GPSfix) {
  if (!ENABLE_SERIAL_DEBUGGING) { return; }
    
  if (GPSfix.valid.lat_err && GPSfix.valid.lon_err) {  
      Serial.printf("latitude error: %f\n", GPSfix.lat_err());      
      Serial.printf("longitude error: %f\n", GPSfix.lon_err());    
  }
  
  if (GPSfix.valid.hdg_err) {  
      Serial.printf("heading error: %f\n", GPSfix.hdg_err());
  }

  if (GPSfix.valid.location) {
		Serial.println("location float");	
		Serial.printf("%f\n", GPSfix.latitude());
    Serial.printf("%f\n", GPSfix.longitude());  			    
	}

	if (GPSfix.valid.date) {
		Serial.println("date");	
		Serial.printf("%d %d %d\n", GPSfix.dateTime.year, GPSfix.dateTime.month, GPSfix.dateTime.date);
	}

	if (GPSfix.valid.time) {
		Serial.println("time");	
		Serial.printf("%d %d %d\n", GPSfix.dateTime.hours, GPSfix.dateTime.minutes, GPSfix.dateTime.seconds);
	}	

	if (GPSfix.valid.speed) {
		Serial.println("speed");	
		Serial.printf("%f KT\n", GPSfix.speed());    
	}

	if (GPSfix.valid.heading) {
		Serial.println("heading");	
		Serial.printf("%f DEG\n", GPSfix.heading());
    Serial.printf("%d DEG\n", GPSfix.heading_cd());    
	}
}

void logToSDcard(String fileName, LinkedList<gps_fix> &positions) {
  int numberOfPositions = positions.size();

  if (numberOfPositions == 0) { return; }

  File file = SD.open(fileName, FILE_WRITE);    
  
  if (!file) { 
    DEBUG("cannot open file");
    positions.clear();  
    return; 
  }

  DEBUG(fileName);  

  for (int i = 0; i < numberOfPositions; ++i) {
    gps_fix current = positions.get(i);

    file.printf(
      "%f,%f,%f,%f,%02d-%02d-%04d,%02d:%02d:%02d\n",
      current.latitude(),
      current.longitude(),
      current.speed(),
      current.heading(),            
      current.dateTime.date,      
      current.dateTime.month,
      current.dateTime.full_year(),
      current.dateTime.hours,
      current.dateTime.minutes,
      current.dateTime.seconds);
	}

  DEBUG(String("written " + String(numberOfPositions) + " files to SD"));
      
  file.close();
  positions.clear();  
}

void drawGPSscreen(GpsFormat GPSformat, bool inMovingState) {    
  Display.clear();    
  
  Display.setTextAlignment(TEXT_ALIGN_LEFT);
  Display.setFont(Monospaced_plain_10);  

  //date time
  Display.drawString(0, 0,  "UTC: " + GPSformat.DateTime);

  //instruments
  Display.drawHorizontalLine(0, 15, 128);    
  Display.drawString(0, 20,  "COG: " + GPSformat.COG); Display.drawCircle(52, 25, 2);
  Display.drawString(60, 20, "SOG: " + GPSformat.SOG);  
  Display.drawVerticalLine(57, 15, 20);

  //position
  Display.drawHorizontalLine(0, 35, 128);    
  Display.drawString(0, 40, "lat: " + GPSformat.Latitude); Display.drawCircle(52, 45, 2);  
  Display.drawString(0, 50, "lng: " + GPSformat.Longitude);  
  Display.drawCircle(52, 55, 2);
  
  //moving state
  if (inMovingState) {
    Display.drawCircle(119, 51, 1);
    Display.drawCircle(119, 51, 3);    
    Display.drawCircle(119, 51, 5);
  }  

  Display.display();
}

GpsFormat formatFix(gps_fix &GPSfix) {
  String dateTime = "--:-- XX-XX-XXXX";
  String cog = "--- ";  
  String sog = "--.-KT";
  String latString = " --  --.--' -";
  String lngString = "---  --.--' -"; 
  
  if (GPSfix.valid.date && GPSfix.valid.time) {
    char timeBuff[40];
    sprintf(timeBuff, "%02d:%02d %02d-%02d-20%02d",
      GPSfix.dateTime.hours,
      GPSfix.dateTime.minutes,      
      GPSfix.dateTime.date,
      GPSfix.dateTime.month,
      GPSfix.dateTime.year
    );
    dateTime = timeBuff;
  }

  if (GPSfix.valid.speed) {
    char cogBuff[40];
    float speedFloat = GPSfix.speed();
    int speed = (int)speedFloat;
    int speedDec = (int) ((speedFloat - speed) * 10);
    sprintf(cogBuff, "%02d.%dKT", speed, speedDec);
    sog = cogBuff;
  }

  if (GPSfix.valid.heading) {
    char headingBuff[40];        
    sprintf(headingBuff, "%03d", (int)GPSfix.heading());
    cog = headingBuff;
  }

  if (GPSfix.valid.location) {
    char latitudeBuff[40];
    char longitudeBuff[40];  

    float latitudeFloat  = GPSfix.latitude();
    float longitudeFloat = GPSfix.longitude();

    char latHem = 'N';
    char lngHem = 'E';

    if (latitudeFloat  < 0) latHem = 'S';
    if (longitudeFloat < 0) lngHem = 'W';

    int latDegrees = abs(latitudeFloat);
    int lngDegrees = abs(longitudeFloat);

    float latMinutesFloat = (abs(latitudeFloat)  - latDegrees) * 60;
    float lngMinutesFloat = (abs(longitudeFloat) - lngDegrees) * 60;

    int latMinutes = (int)latMinutesFloat;
    int lngMinutes = (int)lngMinutesFloat;        

    int latMinutesDecimal =  (int)ceill((latMinutesFloat - latMinutes) * 100);
    int lngMinutesDecimal =  (int)ceill((lngMinutesFloat - lngMinutes) * 100);    

    sprintf(latitudeBuff, " %02d  %02d.%02d' %c", latDegrees, latMinutes, latMinutesDecimal, latHem);
    sprintf(longitudeBuff, "%03d  %02d.%02d' %c", lngDegrees, lngMinutes, lngMinutesDecimal, lngHem);

    latString = latitudeBuff;
    lngString = longitudeBuff;    
  }

  GpsFormat result = {   
    dateTime,
    cog,
    sog, 
    latString,
    lngString    
  };

  return result;
}

bool isValidFix(bool validFixOnlyWithHeading, gps_fix &GPSfix, MotionStateMonitor motionMonitor) {
  bool validHeadingAndSpeed = !validFixOnlyWithHeading || (
    motionMonitor.IsInMovingState()
  );

  return GPSfix.valid.location &&
         GPSfix.valid.date &&
         GPSfix.valid.time &&
         validHeadingAndSpeed;
}

String formatFileName(gps_fix &GPSfix) {
  char timeBuff[40];
  
  sprintf(timeBuff, "%04d-%02d-%02d.csv",         
    GPSfix.dateTime.full_year(),
    GPSfix.dateTime.month, 
    GPSfix.dateTime.date
  );

  return String(timeBuff);
}

void GPSloop() {
  DisplayMonitor.SetState(digitalRead(BUTTON_PIN), Display);

	while (GPS.available(SerialGPS)) {
    GPS.send_P(&SerialGPS, F("PUBX,40,GST,0,1,0,0,0,0"));		    
    DisplayMonitor.SetState(digitalRead(BUTTON_PIN), Display);    

    gps_fix GPSfix = GPS.read();
    MotionMonitor.LogHeadingStatus(GPSfix.valid.heading && GPSfix.valid.speed);
    
    DisplayMonitor.SetState(digitalRead(BUTTON_PIN), Display);

    if (ActionTimings.ShouldDisplay()) {      
      DEBUG("printing to display");
      GpsFormat GPSformat = formatFix(GPSfix);      
      drawGPSscreen(GPSformat, MotionMonitor.IsInMovingState());		      
    }    

    DisplayMonitor.SetState(digitalRead(BUTTON_PIN), Display);

    if (ActionTimings.ShouldLog() && isValidFix(VALID_FIX_ONLY_WITH_HEADING, GPSfix, MotionMonitor)) {
      DEBUG("logging to memory");
      GPSDataList.add(GPSfix);

      if (ActionTimings.ShouldUpdateCard()) {        
        DEBUG("logging SD card");
        logToSDcard(formatFileName(GPSfix), GPSDataList);
      }
    }
      
    DisplayMonitor.SetState(digitalRead(BUTTON_PIN), Display);

    GPSdebug(GPSfix);		            
	}      
}

void loop() {    
  GPSloop();  
}