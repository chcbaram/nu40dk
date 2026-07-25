#include "display.h"




typedef enum
{
  LED_CMD_OFF,
  LED_CMD_ON,
  LED_CMD_TOGGLE,
  LED_CMD_BLINK,
} LedCmd_t;

typedef struct
{
  uint8_t   ch;
  uint8_t   cmd;
  uint32_t  period;
} display_msg_t;

typedef struct
{
  bool      is_blink;
  uint32_t  period;
  uint32_t  next_time;
} led_state_t;


static bool init(void);
static void displayThread(void *arg1, void *arg2, void *arg3);
static void displayCmdExec(display_msg_t *p_msg);
#if CLI_USE(HW_DISPLAY)
static void cliDisplay(cli_args_t *args);
#endif


MODULE_DEF(display)
{
  .name     = "display",
  .priority = MODULE_PRI_NORMAL,
  .init     = init
};


static K_THREAD_STACK_DEFINE(thread_stack, _HW_DEF_RTOS_THREAD_MEM_DISPLAY);
static struct k_thread thread_data;

K_MSGQ_DEFINE(display_msgq, sizeof(display_msg_t), 8, 4);

static led_state_t led_state[LED_MAX_CH];




bool init(void)
{
  k_tid_t tid;


  for (int i=0; i<LED_MAX_CH; i++)
  {
    led_state[i].is_blink  = false;
    led_state[i].period    = 0;
    led_state[i].next_time = 0;
  }

  tid = k_thread_create(&thread_data,
                        thread_stack,
                        K_THREAD_STACK_SIZEOF(thread_stack),
                        displayThread,
                        NULL, NULL, NULL,
                        _HW_DEF_RTOS_THREAD_PRI_DISPLAY, 0, K_NO_WAIT);

  if (tid != NULL)
  {
    k_thread_name_set(tid, "display");
  }

#if CLI_USE(HW_DISPLAY)
  cliAdd("display", cliDisplay);
#endif

  return tid != NULL ? true : false;
}

void displayThread(void *arg1, void *arg2, void *arg3)
{
  display_msg_t msg;


  while(1)
  {
    uint32_t now = millis();
    int32_t  wait_ms = -1;

    // 깜빡임이 필요한 채널만 처리하고, 다음 전환까지 남은 시간을 구한다.
    //
    for (int i=0; i<LED_MAX_CH; i++)
    {
      int32_t remain;

      if (led_state[i].is_blink != true)
      {
        continue;
      }

      if ((int32_t)(now - led_state[i].next_time) >= 0)
      {
        ledToggle(i);
        led_state[i].next_time = now + led_state[i].period;
      }

      remain = (int32_t)(led_state[i].next_time - now);
      if (remain < 0)
      {
        remain = 0;
      }

      if (wait_ms < 0 || remain < wait_ms)
      {
        wait_ms = remain;
      }
    }

    // 할 일이 없으면 명령이 올 때까지 무한 대기한다. (깨어나지 않으므로 전력 소모 없음)
    //
    if (k_msgq_get(&display_msgq, &msg, wait_ms < 0 ? K_FOREVER : K_MSEC(wait_ms)) == 0)
    {
      displayCmdExec(&msg);
    }
  }
}

void displayCmdExec(display_msg_t *p_msg)
{
  uint8_t ch = p_msg->ch;


  if (ch >= LED_MAX_CH)
  {
    return;
  }

  switch(p_msg->cmd)
  {
    case LED_CMD_ON:
      led_state[ch].is_blink = false;
      ledOn(ch);
      break;

    case LED_CMD_OFF:
      led_state[ch].is_blink = false;
      ledOff(ch);
      break;

    case LED_CMD_TOGGLE:
      led_state[ch].is_blink = false;
      ledToggle(ch);
      break;

    case LED_CMD_BLINK:
      if (p_msg->period == 0)
      {
        led_state[ch].is_blink = false;
        ledOff(ch);
      }
      else
      {
        led_state[ch].is_blink  = true;
        led_state[ch].period    = p_msg->period;
        led_state[ch].next_time = millis();
      }
      break;
  }
}

static bool displaySendCmd(uint8_t ch, uint8_t cmd, uint32_t period)
{
  display_msg_t msg;


  if (ch >= LED_MAX_CH) return false;

  msg.ch     = ch;
  msg.cmd    = cmd;
  msg.period = period;

  return k_msgq_put(&display_msgq, &msg, K_NO_WAIT) == 0 ? true : false;
}

bool displayLedOn(uint8_t ch)
{
  return displaySendCmd(ch, LED_CMD_ON, 0);
}

bool displayLedOff(uint8_t ch)
{
  return displaySendCmd(ch, LED_CMD_OFF, 0);
}

bool displayLedToggle(uint8_t ch)
{
  return displaySendCmd(ch, LED_CMD_TOGGLE, 0);
}

bool displayLedBlink(uint8_t ch, uint32_t period_ms)
{
  return displaySendCmd(ch, LED_CMD_BLINK, period_ms);
}


#if CLI_USE(HW_DISPLAY)

void cliDisplay(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info"))
  {
    for (int i=0; i<LED_MAX_CH; i++)
    {
      cliPrintf("led%d : %s", i+1, led_state[i].is_blink ? "blink" : "-");
      if (led_state[i].is_blink == true)
      {
        cliPrintf(" %d ms", led_state[i].period);
      }
      cliPrintf("\n");
    }
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "on"))
  {
    displayLedOn((uint8_t)args->getData(1) - 1);
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "off"))
  {
    displayLedOff((uint8_t)args->getData(1) - 1);
    ret = true;
  }

  if (args->argc == 3 && args->isStr(0, "blink"))
  {
    displayLedBlink((uint8_t)args->getData(1) - 1, (uint32_t)args->getData(2));
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("display info\n");
    cliPrintf("display on  ch[1~%d]\n", LED_MAX_CH);
    cliPrintf("display off ch[1~%d]\n", LED_MAX_CH);
    cliPrintf("display blink ch[1~%d] time_ms\n", LED_MAX_CH);
  }
}

#endif
