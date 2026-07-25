#include "hw.h"




bool hwInit(void)
{
  bspInit();

#ifdef _USE_HW_CLI
  cliInit();
#endif

  resetInit();
#ifdef _USE_HW_UART
  uartInit();
#endif
  ledInit();

#ifdef _USE_HW_CLI
  cliOpen(HW_UART_CH_CLI, 115200);
#endif

  return true;
}
