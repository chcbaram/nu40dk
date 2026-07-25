#include "ap.h"






void apInit(void)
{
}

void apMain(void)
{
  while(1)
  {
    // 1. 순차 점등
    //
    for (int i=0; i<LED_MAX_CH; i++)
    {
      ledOn(i);
      delay(200);
    }

    // 2. 순차 소등
    //
    for (int i=0; i<LED_MAX_CH; i++)
    {
      ledOff(i);
      delay(200);
    }

    // 3. 전체 점멸
    //
    for (int i=0; i<6; i++)
    {
      for (int j=0; j<LED_MAX_CH; j++)
      {
        ledToggle(j);
      }
      delay(300);
    }

    delay(500);
  }
}
