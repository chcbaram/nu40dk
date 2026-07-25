#include "ap_def.h"


// 저전력 빌드에서는 CLI 전체가 빠진다. (Kconfig APP_USE_CLI)
//
#ifdef _USE_HW_CLI


#define CLI_CONNECT_CHECK_MS      500


static bool init(void);
static void cliThread(void *arg1, void *arg2, void *arg3);


MODULE_DEF(cli)
{
  .name     = "cli",
  .priority = MODULE_PRI_LOW,
  .init     = init
};


static K_THREAD_STACK_DEFINE(thread_stack, _HW_DEF_RTOS_THREAD_MEM_CLI);
static struct k_thread thread_data;




bool init(void)
{
  k_tid_t tid;


  cliOpen(HW_UART_CH_CLI, 115200);

  tid = k_thread_create(&thread_data,
                        thread_stack,
                        K_THREAD_STACK_SIZEOF(thread_stack),
                        cliThread,
                        NULL, NULL, NULL,
                        _HW_DEF_RTOS_THREAD_PRI_CLI, 0, K_NO_WAIT);

  if (tid != NULL)
  {
    k_thread_name_set(tid, "cli");
  }

  return tid != NULL ? true : false;
}

void cliThread(void *arg1, void *arg2, void *arg3)
{
  bool is_connected = false;


  while(1)
  {
    // 호스트가 포트를 열지 않았으면 CLI 를 돌리지 않는다.
    // 연결 확인만 주기적으로 하고 나머지 시간은 잠든다.
    //
    if (cdcIsConnect() != true)
    {
      is_connected = false;
      delay(CLI_CONNECT_CHECK_MS);
      continue;
    }

    if (is_connected == false)
    {
      is_connected = true;
      uartFlush(HW_UART_CH_CLI);
    }

    cliMain();

    // 수신이 있을 때만 깨어난다. 타임아웃은 연결 해제를 감지하기 위한 것.
    //
    cdcWaitRx(CLI_CONNECT_CHECK_MS);
  }
}


#endif
