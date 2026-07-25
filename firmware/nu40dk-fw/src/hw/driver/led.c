#include "led.h"


#ifdef _USE_HW_LED
#include "cli.h"
#include <zephyr/drivers/gpio.h>


typedef struct
{
  struct gpio_dt_spec h_dt;
} led_tbl_t;


const led_tbl_t led_tbl[LED_MAX_CH] = {
  {GPIO_DT_SPEC_GET(DT_NODELABEL(led1), gpios)}, // LED1, P0.13
  {GPIO_DT_SPEC_GET(DT_NODELABEL(led2), gpios)}, // LED2, P0.14
  {GPIO_DT_SPEC_GET(DT_NODELABEL(led3), gpios)}, // LED3, P0.15
  {GPIO_DT_SPEC_GET(DT_NODELABEL(led4), gpios)}, // LED4, P0.16
};


#ifdef _USE_HW_CLI
static void cliLed(cli_args_t *args);
#endif


bool ledInit(void)
{
  bool ret = true;


  for (int i=0; i<LED_MAX_CH; i++)
  {
    if (gpio_is_ready_dt(&led_tbl[i].h_dt) != true)
    {
      ret = false;
      continue;
    }

    // 회로도상 Active Low 이므로 극성은 디바이스 트리에서 처리한다.
    //
    if (gpio_pin_configure_dt(&led_tbl[i].h_dt, GPIO_OUTPUT_INACTIVE) < 0)
    {
      ret = false;
    }
  }

#ifdef _USE_HW_CLI
  cliAdd("led", cliLed);
#endif

  return ret;
}

bool ledToSleep(void)
{
  // LED 구동단이 CMOS 인버터(NC7WZ14) 입력이므로 핀을 끊으면 입력이 플로팅되어
  // 관통 전류가 흐른다. 슬립 진입시에도 출력 상태를 유지한 채로 끈다.
  //
  for (int i=0; i<LED_MAX_CH; i++)
  {
    ledOff(i);
  }

  return true;
}

void ledOn(uint8_t ch)
{
  if (ch >= LED_MAX_CH) return;

  gpio_pin_set_dt(&led_tbl[ch].h_dt, 1);
}

void ledOff(uint8_t ch)
{
  if (ch >= LED_MAX_CH) return;

  gpio_pin_set_dt(&led_tbl[ch].h_dt, 0);
}

void ledToggle(uint8_t ch)
{
  if (ch >= LED_MAX_CH) return;

  gpio_pin_toggle_dt(&led_tbl[ch].h_dt);
}





#ifdef _USE_HW_CLI

void cliLed(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 3 && args->isStr(0, "toggle") == true)
  {
    uint8_t  led_ch;
    uint32_t toggle_time;
    uint32_t pre_time;

    led_ch      = (uint8_t)args->getData(1);
    toggle_time = (uint32_t)args->getData(2);

    if (led_ch > 0)
    {
      led_ch--;
    }

    pre_time = millis();
    while(cliKeepLoop())
    {
      if (millis()-pre_time >= toggle_time)
      {
        pre_time = millis();
        ledToggle(led_ch);
      }
    }

    ret = true;
  }


  if (ret != true)
  {
    cliPrintf("led toggle ch[1~%d] time_ms\n", LED_MAX_CH);
  }
}


#endif


#endif
