#include "bsp.h"
#include "uart.h"




// 부팅 로그용. CLI 채널이 없으면(저전력 빌드) 아무것도 하지 않는다.
//
void logPrintf(const char *fmt, ...)
{
#ifdef _USE_HW_UART
  char buf[128];
  va_list args;
  int len;

  va_start(args, fmt);
  len = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  if (len > 0)
  {
    uartWrite(HW_UART_CH_CLI, (uint8_t *)buf, (uint32_t)len);
  }
#else
  (void)fmt;
#endif
}


bool bspInit(void)
{
  bool ret = true;

  return ret;
}

void delay(uint32_t ms)
{
  if (ms > 0)
  {
    k_msleep(ms);
  }
}

uint32_t millis(void)
{
  return k_uptime_get_32();
}



