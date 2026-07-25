#include "uart.h"


#ifdef _USE_HW_UART
#include "cli.h"
#include "qbuffer.h"
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>


#define UART_RX_BUF_LENGTH      256


typedef struct
{
  bool      is_open;
  uint32_t  baud;

  uint8_t   rx_buf[UART_RX_BUF_LENGTH];
  qbuffer_t qbuffer;

  uint32_t  rx_cnt;
  uint32_t  tx_cnt;
} uart_tbl_t;

typedef struct
{
  const char           *p_msg;
  const struct device  *p_dev;
} uart_hw_t;


#if CLI_USE(HW_UART)
static void cliUart(cli_args_t *args);
#endif
static void uartIsrRx(const struct device *p_dev, void *p_arg);


static bool       is_init = false;
static uart_tbl_t uart_tbl[UART_MAX_CH];

const static uart_hw_t uart_hw_tbl[UART_MAX_CH] =
  {
    {"uart0 CLI  ", DEVICE_DT_GET(DT_NODELABEL(uart0))},
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
  const struct device *p_dev;


  if (ch >= UART_MAX_CH) return false;

  if (uart_tbl[ch].is_open == true && uart_tbl[ch].baud == baud)
  {
    return true;
  }

  p_dev = uart_hw_tbl[ch].p_dev;

  if (device_is_ready(p_dev) != true)
  {
    return false;
  }

  switch(ch)
  {
    case _DEF_UART1:
      qbufferCreate(&uart_tbl[ch].qbuffer, &uart_tbl[ch].rx_buf[0], UART_RX_BUF_LENGTH);

      if (uart_irq_callback_user_data_set(p_dev, uartIsrRx, (void *)(uint32_t)ch) == 0)
      {
        uart_irq_rx_enable(p_dev);

        uart_tbl[ch].baud    = baud;
        uart_tbl[ch].is_open = true;
        ret = true;
      }
      break;
  }

  return ret;
}

bool uartClose(uint8_t ch)
{
  if (ch >= UART_MAX_CH) return false;

  if (uart_tbl[ch].is_open == true)
  {
    uart_irq_rx_disable(uart_hw_tbl[ch].p_dev);
    uart_tbl[ch].is_open = false;
  }

  return true;
}

void uartIsrRx(const struct device *p_dev, void *p_arg)
{
  uint8_t ch = (uint8_t)(uint32_t)p_arg;
  uint8_t rx_data;


  if (uart_irq_update(p_dev) != 1)
  {
    return;
  }

  while(uart_irq_rx_ready(p_dev))
  {
    if (uart_fifo_read(p_dev, &rx_data, 1) == 1)
    {
      qbufferWrite(&uart_tbl[ch].qbuffer, &rx_data, 1);
    }
  }
}

uint32_t uartAvailable(uint8_t ch)
{
  uint32_t ret = 0;


  if (ch >= UART_MAX_CH) return 0;

  switch(ch)
  {
    case _DEF_UART1:
      ret = qbufferAvailable(&uart_tbl[ch].qbuffer);
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
    case _DEF_UART1:
      if (qbufferRead(&uart_tbl[ch].qbuffer, &ret, 1) == true)
      {
        uart_tbl[ch].rx_cnt++;
      }
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
    case _DEF_UART1:
      for (uint32_t i=0; i<length; i++)
      {
        uart_poll_out(uart_hw_tbl[ch].p_dev, p_data[i]);
      }
      ret = length;
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
  if (ch >= UART_MAX_CH) return 0;

  return uart_tbl[ch].baud;
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
      cliPrintf("_DEF_UART%d : %s, %d bps\n", i+1, uart_hw_tbl[i].p_msg, uartGetBaud(i));
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
