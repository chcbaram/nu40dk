#include "cdc.h"


#ifdef _USE_HW_CDC
#include "qbuffer.h"
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>


#define CDC_RX_BUF_LENGTH       256


static void cdcIsrRx(const struct device *p_dev, void *p_arg);


static bool      is_init = false;
static uint8_t   rx_buf[CDC_RX_BUF_LENGTH];
static qbuffer_t rx_q;

const static struct device *p_cdc_dev = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));


bool cdcInit(void)
{
  if (is_init == true)
  {
    return true;
  }

  if (device_is_ready(p_cdc_dev) != true)
  {
    return false;
  }

  qbufferCreate(&rx_q, &rx_buf[0], CDC_RX_BUF_LENGTH);

  if (uart_irq_callback_user_data_set(p_cdc_dev, cdcIsrRx, NULL) != 0)
  {
    return false;
  }
  uart_irq_rx_enable(p_cdc_dev);

  is_init = true;

  return true;
}

bool cdcIsInit(void)
{
  return is_init;
}

// 호스트가 포트를 열었는지(DTR) 확인한다.
// USB 미연결 상태에서는 항상 false 이므로 CLI 를 쉬게 하는 판단에 쓴다.
//
bool cdcIsConnect(void)
{
  uint32_t dtr = 0;


  if (is_init != true)
  {
    return false;
  }

  if (uart_line_ctrl_get(p_cdc_dev, UART_LINE_CTRL_DTR, &dtr) != 0)
  {
    return false;
  }

  return dtr ? true : false;
}

void cdcIsrRx(const struct device *p_dev, void *p_arg)
{
  uint8_t rx_data[64];
  int     rx_len;


  // uart_irq_update() 가 rx/tx 플래그를 다시 계산하므로 매 루프마다 호출해야 한다.
  // 루프 밖에서 한 번만 부르면 버퍼를 비운 뒤에도 rx_ready 가 참으로 남아
  // 무한 루프가 된다.
  //
  while(uart_irq_update(p_dev) && uart_irq_is_pending(p_dev))
  {
    if (uart_irq_rx_ready(p_dev) != 1)
    {
      break;
    }

    rx_len = uart_fifo_read(p_dev, rx_data, sizeof(rx_data));
    if (rx_len <= 0)
    {
      break;
    }

    qbufferWrite(&rx_q, rx_data, (uint32_t)rx_len);
  }
}

uint32_t cdcAvailable(void)
{
  if (is_init != true) return 0;

  return qbufferAvailable(&rx_q);
}

uint8_t cdcRead(void)
{
  uint8_t ret = 0;


  if (is_init != true) return 0;

  qbufferRead(&rx_q, &ret, 1);

  return ret;
}

uint32_t cdcWrite(uint8_t *p_data, uint32_t length)
{
  if (cdcIsConnect() != true)
  {
    return 0;
  }

  for (uint32_t i=0; i<length; i++)
  {
    uart_poll_out(p_cdc_dev, p_data[i]);
  }

  return length;
}

uint32_t cdcGetBaud(void)
{
  uint32_t baud = 0;


  if (is_init != true) return 0;

  // CDC 는 호스트가 지정한 값을 그대로 돌려준다.
  //
  if (uart_line_ctrl_get(p_cdc_dev, UART_LINE_CTRL_BAUD_RATE, &baud) != 0)
  {
    return 0;
  }

  return baud;
}

uint8_t cdcGetType(void)
{
  return 0;
}


#endif
