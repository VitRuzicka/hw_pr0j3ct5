#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <time.h>

#define LATCH 0
#define CLK 4
#define DAT 5
#define PWM 16
#define DELAY 50 //let the cap settle
#define INTERVAL 1000000

// WiFi credentials removed - now using WiFiManager

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;



time_t now;                          // this are the seconds since Epoch (1970) - UTC
tm tm;                             // the structure tm holds time information in a more convenient way *


int preloz(int number, int index) {  //returns the n-th index of number ( for number 1234 and index 0 returns 4)
    int numDigits = log10(number) + 1;
    if (index < 0 || index >= numDigits) {
        return 10; // Index out of bounds
    }

    int divisor = pow(10, index);
    return (number / divisor) % 10;
}
void loading(){ //loading animation when connecting to wifi
  // byte segments[] = { //old round loading
  //     B0000001,
  //     B00000010,
  //     B00000100,
  //     B00001000,
  //     B00010000,
  //     B00100000
  //   };
  byte segments[] = { //8 fig loading
    // xGFEDCBA
      B00000001,
      B00000010,
      B01000000,
      B00010000,
      B00001000,
      B00000100,
      B01000000,
      B00100000

    };

  
  for(uint8_t i=0; i < sizeof(segments)/sizeof(segments[0]); i++){ //cycle through all segments
    digitalWrite(LATCH, LOW);
    delay(DELAY);
    for(int u = 0; u < 4; u++){ //shift to all 4 digits
        shiftOut(DAT, CLK, MSBFIRST, segments[i]);
    }
    digitalWrite(LATCH, HIGH);
    delay(DELAY);
    }  
}
void zobraz(int cislo, int dot){ //shows number in argument on the display, param - whether to show dot or not

  byte segments[] = {
    B00111111,  // Digit 0
    B00000110,  // Digit 1
    B01011011,  // Digit 2
    B01001111,  // Digit 3
    B01100110,  // Digit 4
    B01101101,  // Digit 5
    B01111101,  // Digit 6
    B00000111,  // Digit 7
    B01111111,  // Digit 8
    B01101111,   // Digit 9
    B00000000
  };
byte dotSegments[] = { 
    B10111111,  // Digit 0
    B10000110,  // Digit 1
    B11011011,  // Digit 2
    B11001111,  // Digit 3
    B11100110,  // Digit 4
    B11101101,  // Digit 5
    B11111101,  // Digit 6
    B10000111,  // Digit 7
    B11111111,  // Digit 8
    B11101111,   // Digit 9
    B00000000
  };

  digitalWrite(LATCH, LOW);
  delay(DELAY);
  shiftOut(DAT, CLK, MSBFIRST, segments[preloz(cislo, 3)]);
  if(dot){
    shiftOut(DAT, CLK, MSBFIRST, dotSegments[preloz(cislo, 2)]);
  }else{
    shiftOut(DAT, CLK, MSBFIRST, segments[preloz(cislo, 2)]);
  }
  shiftOut(DAT, CLK, MSBFIRST, segments[preloz(cislo, 1)]);
  shiftOut(DAT, CLK, MSBFIRST, segments[preloz(cislo, 0)]);
  
  digitalWrite(LATCH, HIGH);
  delay(DELAY);
}

void setTimezone(String timezone){
  //Serial.printf("  Setting Timezone to %s\n",timezone.c_str());
  setenv("TZ",timezone.c_str(),1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}

void setup(){
  Serial.begin(9600);
  
  // Setup WiFiManager
  WiFiManager wifiManager;
  
  // Uncomment this line to reset saved WiFi credentials (for testing)
  // wifiManager.resetSettings();
  
  // Setup pins before WiFi connection
  pinMode(LATCH, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(DAT, OUTPUT);
  analogWrite(PWM, 255); //full blast
  
  // Show loading animation while attempting to connect
  // If it fails to connect, it will start an access point with name "NTP_Clock_Setup"
  // Connect to it and configure WiFi credentials through the captive portal
  while (WiFi.status() != WL_CONNECTED) {
    loading();
    
    // Try to autoconnect. If it fails, start config portal
    if (!wifiManager.autoConnect("NTP_Clock_Setup")) {
      Serial.println("Failed to connect and hit timeout");
      // Reset and try again
      ESP.restart();
      delay(1000);
    }
  }
  
  Serial.println("Connected to WiFi!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //connect to NTP server 
  setTimezone("CET-1CEST,M3.5.0,M10.5.0/3"); //https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

}

void loop() {

  time(&now); // read the current time
  localtime_r(&now, &tm);     


  int A,B;
  
  if(tm.tm_hour > 19 || 8 > tm.tm_hour ) {  //time based dimming
    analogWrite(PWM, 3);   
  }else{
    analogWrite(PWM, 200);
  }
    
  A = tm.tm_hour * 100 + tm.tm_min; //one int being printed on the display
  B = tm.tm_sec;
  

  if((B % 2) == 0)
  {
    zobraz(A, 0); //not showing dot
  }
  else
  {
    zobraz(A, 1); //showing dot
  }
  Serial.printf("time is %d:%d\n", tm.tm_hour, tm.tm_min);
}
