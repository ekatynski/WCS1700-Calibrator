#include <Arduino.h>
#include <SPI.h>
#include <vector>
#include "Mcp320x.h"

// SPI bus variables
const int WCS_CS = 10;          // WCS1700 ADC CS pin
const int TAMURA_CS = 11;       // Tamura hall effect sensor CS pin
const int ADC_VREF = 4500;      // 4.5V Vref
const int ADC_CLK = 500000;     // SPI clock 500kHz
const int SPLS = 10;            // 10 samples

// ADC autocalibration variables
const int gainChannel = 0;      // ADC channel used to measure gain error
const int offsetChannel = 7;    // ADC channel used to measure offset error
const int errorThreshold = 500; // number of reporting cycles performed before evaluating error
int reportCount = 0;            // running counter for reports
float gain = 1.00;              // ADC gain, set by hand for now
int offset = 0;                 // ADC offset (bits), initially set to zero

// averaging structures
// TODO: set up individual structures for separate ADCs
uint16_t data[SPLS] = {0};      // array to store samples from each adc.read() call
int avg[8] = {0};

// indexing channel enums to integers to ease looping
MCP3208::Channel channels[8] = {
  MCP3208::Channel::SINGLE_0,
  MCP3208::Channel::SINGLE_1,
  MCP3208::Channel::SINGLE_2,
  MCP3208::Channel::SINGLE_3,
  MCP3208::Channel::SINGLE_4,
  MCP3208::Channel::SINGLE_5,
  MCP3208::Channel::SINGLE_6,
  MCP3208::Channel::SINGLE_7
};

// TODO: Initialize and name second ADC for external Hall Effect Sensor
MCP3208 adc(ADC_VREF, WCS_CS);

// calculate and current gain coefficient
float checkGain() {
  adc.read(channels[gainChannel], data);

  // average the samples collected for each channel
    for (int i = 0; i < SPLS; i++) {
      avg[gainChannel] += data[i];
    }
  avg[gainChannel] /= SPLS;

  // divide calculated GAIN_REF voltage by actual to determine gain coefficient
  return float(adc.toAnalog(avg[gainChannel])) / (ADC_VREF / 2);
}

// calculate and return current voltage offset
int checkOffset() {
  adc.read(channels[offsetChannel], data);

  // average the samples collected for each channel
    for (int i = 0; i < SPLS; i++) {
      avg[offsetChannel] += data[i];
    }
  avg[offsetChannel] /= SPLS;

  return avg[offsetChannel];
}

void setup() {
  // configure PIN mode
  pinMode(WCS_CS, OUTPUT);
  pinMode(TAMURA_CS, OUTPUT);

  // set initial PIN state
  digitalWrite(WCS_CS, HIGH);
  digitalWrite(TAMURA_CS, HIGH);

  // initialize serial
  Serial.begin(115200);
  Serial.println("Starting up...");

  // initialize SPI interface for MCP3208
  SPISettings settings(ADC_CLK, MSBFIRST, SPI_MODE3);
  SPI.begin();
  SPI.beginTransaction(settings);

   // report gain and offset
  Serial.println("Gain: " + String(gain) + "\tOffset: " + String(adc.toAnalog(offset)) + " mV");

  // prepare to report
  Serial.println("\t");
  delay(1000);
}

void loop() {
  // check if threshold reached for resetting gain/offset values
  if (reportCount >= errorThreshold) {
    gain = checkGain();
    offset = checkOffset();
    reportCount = 0;

    // print out new gain and offset values
    Serial.println("\nGain: " + String(gain) + "\tOffset: " + String(adc.toAnalog(offset)) + " mV");
    Serial.println("\n");
  }

  // timestamping
  Serial.println("Sample time: " + String((float(millis()) / 1000.0f)) + "s");
  
  // cycle through each channel, taking samples on each
  for (int i = 0; i < 8; i++) {
    // start sampling
    adc.read(channels[i], data);
    
    // average the samples collected for each channel
    for (int j = 0; j < SPLS; j++) {
      avg[i] += data[j];
    }
    avg[i] /= SPLS;
  
    // correct for gain and scale error
    avg[i] = uint16_t(float(avg[i] - offset) / gain);
  }

  // print out the averages for each channel
  for (int i = 0; i < 8; i++) {
    Serial.println("Channel " + String(i) + ":\t" + String(adc.toAnalog(avg[i])) + " mV\t(" + String(avg[i]) + ")");
  }

  reportCount++;  // increment report counter
  Serial.println();
  delay(100);
}