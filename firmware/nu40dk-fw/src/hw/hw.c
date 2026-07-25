#include "hw.h"




bool hwInit(void)
{  
  bspInit();

  resetInit();
  ledInit();

  return true;
}