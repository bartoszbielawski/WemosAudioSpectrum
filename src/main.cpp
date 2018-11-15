#include <Arduino.h>

#include <array>

#include <LEDMatrixDriver.hpp>

#include <math.h>

#include "coeffs.h"

int16_t buffer[dataSize * 2];

LEDMatrixDriver lmd(4, 2);

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


void setup() 
{
  Serial.begin(115200);
  Serial.println("Hello!");
  lmd.setEnabled(true);
  lmd.setIntensity(2);
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
  std::array<int16_t, filters> results;
  // For each of the filters
  for (int f = 0; f < filters; f++)
  { 
    int16_t M = -32000;
    // Calculate responses for samples
    for (int i = 0; i < dataSize; i++)
    {
      int16_t v = filter(buffer + i, coeffs[f], dataSize);
      if (v > M)  // Find new top value
        M = v;
    }

    //calculate overall top value
    results[f] = M;
  }

  for (int i = 0; i < filters; i++)
  {
    results[i] = (logf(results[i]+1)-1) * 3;
    results[i] = min(results[i], (int16_t)8);
    results[i] = max(results[i], (int16_t)0);
    //Serial.println(results[i]);
  }
  
  lmd.clear();
  
  static const uint8_t lookup[9] = {0, 1, 3, 7, 15, 31, 63, 127, 255};

  for (int f = 0; f < filters; f++)
  {
    int d = results[f];
    lmd.setColumn(31-f, lookup[d]);
  }

  
  lmd.display();
}