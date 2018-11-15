#include <Arduino.h>

#include <array>

#include <Adafruit_NeoPixel.h>
#include <MLEDScroll.h>

#include <math.h>

#include "coeffs.h"

int16_t buffer[dataSize * 2];

MLEDScroll matrix(1, D7, D5, true);

static inline unsigned get_ccount(void)
{
       unsigned r;
       asm volatile ("rsr %0, ccount" : "=r"(r));
       return r;
}

int16_t filter(int16_t* data, int16_t* coeffs, int len)
{
  int32_t sum = 0;
  for (int i = 0; i < len; i++)
    sum += (int16_t)pgm_read_word(coeffs + i) * data[i];
  return (sum + 0x8000) >> 16;
}


Adafruit_NeoPixel pixels(1, D2, NEO_GRB + NEO_KHZ800);

void setup() 
{
  Serial.begin(115200);
  Serial.println("Hello!");
  matrix.begin();
  pinMode(D2, OUTPUT);
  pixels.begin();
}

void loop() 
{
  
  // Acquire data and calculate sum
  int32_t sum = 0;
  long start = get_ccount();
  for (int i = 0; i < dataSize * 2; i++)
  {
    int16_t v = analogRead(A0);
    buffer[i] = v;
    sum += v;
  }

  long stop = get_ccount();
  // Enable this to be able to calculate sampling rate
  //Serial.printf("Acq Time: %d\n", stop-start);
  

  // Remove DC offset
  int16_t avg = sum / dataSize;
  for (int i = 0; i < dataSize; i++)
  {
    buffer[i] -= avg;
  }

  // Prepare top values
  std::array<float, 8> results;
  int MM = -10000;
  // For each of the filters
  for (int f = 0; f < 8; f++)
  { 
    float M = -1000000.0;
    // Calculate responses for samples
    for (int i = 0; i < dataSize; i++)
    {
      int16_t v = filter(buffer + i, coeffs[f], dataSize);
      if (v > M)  // Find new top value
        M = v;
    }

    //calculate overall top value
    results[f] = M;
    if (M > MM)
      MM = M;
  }

  for (int i = 0; i < 8; i++)
  {
    results[i] = (logf(results[i]+1)-1) * 3;
    if (results[i] < 0) results[i] = 0;
    if (results[i] > 8) results[i] = 8;
  }
  
  matrix.clear();

  static const uint8_t lookup[9] = {0, 1, 3, 7, 15, 31, 63, 127, 255};

  for (int f = 0; f < 8; f++)
  {
    int d = results[f];
    matrix.disBuffer[7-f] = lookup[d];
    matrix.disBuffer[15-f] = lookup[d];
  }

  matrix.display();

  if (MM>255)
    MM = 255;

  pixels.setPixelColor(0, pixels.Color(MM,0,MM)); // Moderately bright green color
  pixels.show();
}