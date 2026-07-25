#include "hw.h"




bool hwInit(void)
{
  bspInit();

  cliInit();

  resetInit();
  uartInit();
  ledInit();

  cliOpen(HW_UART_CH_CLI, 115200);

  return true;
}
