#include "ap.h"




void apInit(void)
{
}

void apMain(void)
{
  uint32_t pre_time = millis();
#ifdef _USE_HW_CLI
  bool is_connected = false;
#endif


  while(1)
  {
    if (millis()-pre_time >= 500)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);
    }

#ifdef _USE_HW_CLI
    // USB 로 포트가 열려 있을 때만 CLI 를 돌린다.
    // 미연결 상태에서는 100ms 주기로만 깨어나 소비전류를 낮춘다.
    //
    if (cdcIsConnect() == true)
    {
      if (is_connected == false)
      {
        is_connected = true;
        uartFlush(HW_UART_CH_CLI);
      }

      cliMain();
      delay(1);
    }
    else
    {
      is_connected = false;
      delay(100);
    }
#else
    delay(100);
#endif
  }
}
