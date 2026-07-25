#include "ap.h"
#include "display/display.h"

#ifdef CONFIG_APP_POWER_TEST
#include <zephyr/sys/poweroff.h>
#endif




void apInit(void)
{
  moduleInit();
}

void apMain(void)
{
#ifdef CONFIG_APP_USE_HEARTBEAT_LED
  // 하트비트 LED. 실측 기준 LED 1개가 1~2mA 를 먹으므로 전류 측정시에는 끈다.
  //
  displayLedBlink(_DEF_LED1, 500);
#else
  displayLedOff(_DEF_LED1);
#endif

#ifdef CONFIG_APP_POWER_TEST
  // 소비전류 바닥값 측정. 재플래시할 여유를 두고 System OFF 로 들어간다.
  //
  delay(3000);
  ledOff(_DEF_LED1);
  sys_poweroff();
#endif

  while(1)
  {
    // 각 모듈이 자기 스레드에서 이벤트 기반으로 동작한다.
    // 메인은 할 일이 없으므로 계속 잠들어 있는다.
    //
    delay(1000);
  }
}
