#include "uart.h"


#ifdef _USE_HW_UART
#include "cli.h"
#include "cdc.h"


typedef struct
{
  bool      is_open;
  uint32_t  baud;

  uint32_t  rx_cnt;
  uint32_t  tx_cnt;
} uart_tbl_t;

typedef struct
{
  const char *p_msg;
} uart_hw_t;


#if CLI_USE(HW_UART)
static void cliUart(cli_args_t *args);
#endif


static bool       is_init = false;
static uart_tbl_t uart_tbl[UART_MAX_CH];

const static uart_hw_t uart_hw_tbl[UART_MAX_CH] =
  {
    {"USB CDC "},
  };


bool uartInit(void)
{
  for (int i=0; i<UART_MAX_CH; i++)
  {
    uart_tbl[i].is_open = false;
    uart_tbl[i].baud    = 115200;
    uart_tbl[i].rx_cnt  = 0;
    uart_tbl[i].tx_cnt  = 0;
  }

  is_init = true;

#if CLI_USE(HW_UART)
  cliAdd("uart", cliUart);
#endif

  return true;
}

bool uartDeInit(void)
{
  for (int i=0; i<UART_MAX_CH; i++)
  {
    uartClose(i);
  }

  return true;
}

bool uartIsInit(void)
{
  return is_init;
}

bool uartOpen(uint8_t ch, uint32_t baud)
{
  bool ret = false;


  if (ch >= UART_MAX_CH) return false;

  if (uart_tbl[ch].is_open == true && uart_tbl[ch].baud == baud)
  {
    return true;
  }

  switch(ch)
  {
    case HW_UART_CH_USB:
      ret = cdcInit();
      if (ret == true)
      {
        uart_tbl[ch].baud    = baud;
        uart_tbl[ch].is_open = true;
      }
      break;
  }

  return ret;
}

bool uartClose(uint8_t ch)
{
  if (ch >= UART_MAX_CH) return false;

  uart_tbl[ch].is_open = false;

  return true;
}

uint32_t uartAvailable(uint8_t ch)
{
  uint32_t ret = 0;


  if (ch >= UART_MAX_CH) return 0;

  switch(ch)
  {
    case HW_UART_CH_USB:
      ret = cdcAvailable();
      break;
  }

  return ret;
}

bool uartFlush(uint8_t ch)
{
  uint32_t pre_time;


  pre_time = millis();
  while(uartAvailable(ch))
  {
    if (millis()-pre_time >= 10)
    {
      break;
    }
    uartRead(ch);
  }

  return true;
}

uint8_t uartRead(uint8_t ch)
{
  uint8_t ret = 0;


  if (ch >= UART_MAX_CH) return 0;

  switch(ch)
  {
    case HW_UART_CH_USB:
      ret = cdcRead();
      uart_tbl[ch].rx_cnt++;
      break;
  }

  return ret;
}

uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t length)
{
  uint32_t ret = 0;


  if (ch >= UART_MAX_CH) return 0;
  if (uart_tbl[ch].is_open != true) return 0;

  switch(ch)
  {
    case HW_UART_CH_USB:
      ret = cdcWrite(p_data, length);
      break;
  }
  uart_tbl[ch].tx_cnt += ret;

  return ret;
}

uint32_t uartPrintf(uint8_t ch, const char *fmt, ...)
{
  char buf[256];
  va_list args;
  int len;
  uint32_t ret;


  va_start(args, fmt);
  len = vsnprintf(buf, 256, fmt, args);

  ret = uartWrite(ch, (uint8_t *)buf, len);

  va_end(args);

  return ret;
}

uint32_t uartGetBaud(uint8_t ch)
{
  uint32_t ret = 0;


  if (ch >= UART_MAX_CH) return 0;

  switch(ch)
  {
    case HW_UART_CH_USB:
      ret = cdcGetBaud();
      break;
  }

  if (ret == 0)
  {
    ret = uart_tbl[ch].baud;
  }

  return ret;
}

uint32_t uartGetRxCnt(uint8_t ch)
{
  if (ch >= UART_MAX_CH) return 0;

  return uart_tbl[ch].rx_cnt;
}

uint32_t uartGetTxCnt(uint8_t ch)
{
  if (ch >= UART_MAX_CH) return 0;

  return uart_tbl[ch].tx_cnt;
}


#if CLI_USE(HW_UART)

void cliUart(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info"))
  {
    for (int i=0; i<UART_MAX_CH; i++)
    {
      cliPrintf("_DEF_UART%d : %s, %d bps, %s\n",
                i+1,
                uart_hw_tbl[i].p_msg,
                uartGetBaud(i),
                cdcIsConnect() ? "connected" : "disconnected");
      cliPrintf("             rx %d, tx %d\n", uartGetRxCnt(i), uartGetTxCnt(i));
    }
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("uart info\n");
  }
}

#endif


#endif
