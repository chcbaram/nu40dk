#include "ap.h"






void apInit(void)
{  
}

void apMain(void)
{
  while(1)
  {
    ledToggle(_DEF_LED1);
    ledToggle(_DEF_LED2);
    delay(500);    
  }
}

