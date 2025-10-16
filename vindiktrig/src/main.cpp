#include <Arduino.h>
#include <FastLED.h>
#include <array>
#include <algorithm>

// FastLED Configuration
#define LED_PIN 25        // Data pin for LEDs
#define NUM_LEDS 3        // Number of LEDs
#define LED_TYPE WS2812B  // LED strip type (WS2812B, WS2811, etc.)
#define COLOR_ORDER GRB   // Color order for your LED strip
#define BRIGHTNESS 64     // LED brightness (0-255)
#define MAX_BRIGHT 128    // Maximum brightness for breathing effect
#define SENSOR_STARTUP_TIME 5*60*1000 //5 minutes for sensor to stabilize
CRGB leds[NUM_LEDS];      // LED array

// MH-Z14 CO2 Sensor Configuration
#define MHZ14_RX 16  // ESP32 RX connected to MH-Z14 TX
#define MHZ14_TX 17  // ESP32 TX connected to MH-Z14 RX
#define MHZ14_BAUDRATE 9600
unsigned long lastMeas = 0;
unsigned long lastBr = 0;
bool dir = true; //direction of breathing animation
int co2 = 0; //current CO2 value
uint8_t brightness = 0;
// MH-Z14 Commands

// my approach at lookup table
template <typename ARGUMENT_TYPE> struct lookup_table_data{
  ARGUMENT_TYPE argument;
  const CRGB color;
};

constexpr std::array<lookup_table_data<int>, 5> co2_lookup_table = {{
  {400, CRGB::Green},
  {800, CRGB::Yellow},
  {1200, CRGB::Orange},
  {2000, CRGB::Red},
  {3000, CRGB::Purple}
}};

template<typename value_type, typename key_type, std::size_t TableSize>
value_type lookupInTable(key_type const current_value, std::array<lookup_table_data<key_type>, TableSize> const& lookup_table) {

  if (current_value <= lookup_table.front().argument) {
    return lookup_table.front().color;
  }
  if (current_value >= lookup_table.back().argument) {
    return lookup_table.back().color;
  }

  // Binary search for lowest known value greater or equal to current value
  auto const closestHigher = std::ranges::upper_bound(lookup_table, current_value, std::less{}, &lookup_table_data<key_type>::argument);
  assert(closestHigher != lookup_table.end());

  // perform linear interpolation on values from intervals
  auto const [low, colorAtLow] = *std::prev(closestHigher);
  auto const [high, colorAtHigh] = *closestHigher;
  
  // Calculate interpolation factor (0.0 to 1.0)
  float t = (float)(current_value - low) / (float)(high - low);
  
  // Linear interpolation for each RGB component
  CRGB result;
  result.r = colorAtLow.r + (colorAtHigh.r - colorAtLow.r) * t;
  result.g = colorAtLow.g + (colorAtHigh.g - colorAtLow.g) * t;
  result.b = colorAtLow.b + (colorAtHigh.b - colorAtLow.b) * t;
  
  return result;
}


const byte MHZ14_CMD_READ_CO2[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};

// Function declarations
void sendCommand(const byte* cmd, byte* response);
int readCO2();
byte calculateChecksum(byte* data);

void setup() {
  // Initialize USB Serial for debugging
  Serial.begin(115200);
  delay(1000);
  Serial.println("MH-Z14 CO2 Sensor with FastLED");
  
  // Initialize FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(0);
  
  // Turn all LEDs off initially
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  
  // Initialize Hardware Serial for MH-Z14
  // Using Serial2 (UART2) on ESP32
  Serial2.begin(MHZ14_BAUDRATE, SERIAL_8N1, MHZ14_RX, MHZ14_TX);
  delay(2000); // Wait for sensor to warm up
  
  Serial.println("MH-Z14 initialized. Waiting for sensor warm-up...");
  
  // LED Animation: Rise from bottom to top with increasing brightness
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.setBrightness(MAX_BRIGHT);
  
  // Phase 1: Fade in from bottom to top (LED 0 -> LED 1 -> LED 2)
  int fadeSteps = 85;  // Steps per LED fade-in
  CRGB color = CRGB::Green;  // You can change this color
  
  for(int step = 0; step < fadeSteps * NUM_LEDS; step++) {
    // Calculate which LEDs should be lit and their brightness
    for(int led = 0; led < NUM_LEDS; led++) {
      int ledStartStep = led * fadeSteps;
      
      if(step >= ledStartStep) {
        // Calculate brightness for this LED (0-255)
        int brightness = min(255, (step - ledStartStep) * 3);
        leds[led] = color;
        leds[led].fadeToBlackBy(255 - brightness);
      } else {
        leds[led] = CRGB::Black;
      }
    }
    
    FastLED.show();
    delay(15);
  }
  
  // Phase 2: Hold at full brightness
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
  delay(2000);
  
  // Phase 3: Fade out all LEDs together slowly
  for(int brightness = 255; brightness >= 0; brightness -= 2) {
    for(int led = 0; led < NUM_LEDS; led++) {
      leds[led] = color;
      leds[led].fadeToBlackBy(255 - brightness);
    }
    FastLED.show();
    delay(15);
  }
  
  // Turn off all LEDs
  fill_solid(leds, NUM_LEDS, CRGB::Orange);
  FastLED.setBrightness(0);
  FastLED.show();

}



void loop() {
  if(millis() - lastMeas >= 5000){  //run every 5s to read the sensor
    lastMeas = millis();
    co2 = readCO2();
    if (co2 >= 0) {
      Serial.print("CO2: ");
      Serial.print(co2);
      Serial.println(" ppm");
    } else {
      Serial.println("Error reading CO2 sensor");
    }
  }
  
  
  //breathing animation
  if(millis() < SENSOR_STARTUP_TIME && millis()-lastBr >= 10){
    lastBr = millis();
    if(dir){
      brightness++;
      if(brightness >= MAX_BRIGHT){
        dir = false;
      }
    } else {
      brightness--;
      if(brightness <= 0){
        dir = true;
      }
    }
    FastLED.setBrightness(brightness);
    FastLED.show();
  } else if(millis() >= SENSOR_STARTUP_TIME){ //then show the bar according to the CO2 value
    // TODO: Add code here to show CO2 levels on LED bar
    for(int i = 0; i < NUM_LEDS; i++) {
      leds[i] = lookupInTable<CRGB>(co2, co2_lookup_table);
    }
    //TODO: add brightness control based on daytime
    FastLED.show();
  }
}

// Function to send command and receive response from MH-Z14
void sendCommand(const byte* cmd, byte* response) {
  // Clear serial buffer
  while (Serial2.available()) {
    Serial2.read();
  }
  
  // Send command
  Serial2.write(cmd, 9);
  Serial2.flush();
  
  // Wait for response
  unsigned long timeout = millis();
  int index = 0;
  
  while (index < 9) {
    if (Serial2.available()) {
      response[index++] = Serial2.read();
    }
    
    // Timeout after 1 second
    if (millis() - timeout > 1000) {
      Serial.println("Timeout waiting for sensor response");
      break;
    }
  }
}

// Function to read CO2 value from MH-Z14
int readCO2() {
  byte response[9];
  
  sendCommand(MHZ14_CMD_READ_CO2, response);
  
  // Verify response
  if (response[0] == 0xFF && response[1] == 0x86) {
    // Calculate checksum
    byte checksum = calculateChecksum(response);
    
    if (checksum == response[8]) {
      // Extract CO2 value (High byte and Low byte)
      int co2 = (int)response[2] * 256 + (int)response[3];
      return co2;
    } else {
      Serial.println("Checksum error");
      return -1;
    }
  } else {
    Serial.println("Invalid response header");
    return -1;
  }
}

// Calculate checksum for MH-Z14 response
byte calculateChecksum(byte* data) {
  byte checksum = 0;
  for (int i = 1; i < 8; i++) {
    checksum += data[i];
  }
  checksum = 0xFF - checksum;
  checksum += 1;
  return checksum;
}