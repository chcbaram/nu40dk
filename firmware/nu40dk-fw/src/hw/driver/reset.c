#include "reset.h"


#ifdef _USE_HW_RESET
#include "cli.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <hal/nrf_power.h>


// Adafruit nRF52 부트로더가 GPREGRET[0]으로 판단하는 DFU 진입 매직
//
#define DFU_MAGIC_OTA_RESET         0xA8    // BLE OTA DFU
#define DFU_MAGIC_SERIAL_ONLY_RESET 0x4E    // USB CDC DFU
#define DFU_MAGIC_UF2_RESET         0x57    // USB UF2 드래그앤드롭

// GPREGRET[0]은 부트로더가 사용하므로 앱의 부트 모드는 GPREGRET[1]에 저장한다.
//
#define GPREGRET_DFU                0
#define GPREGRET_BOOT_MODE          1


static uint32_t reset_bits = 0;

#if CLI_USE(HW_RESET)
static void cliReset(cli_args_t *args);
#endif


bool resetInit(void)
{
  uint32_t resetreas;


  resetreas = nrf_power_resetreas_get(NRF_POWER);
  nrf_power_resetreas_clear(NRF_POWER, resetreas);

  if (resetreas == 0)
  {
    reset_bits |= (1<<RESET_BIT_POWER);
  }
  if (resetreas & NRF_POWER_RESETREAS_RESETPIN_MASK)
  {
    reset_bits |= (1<<RESET_BIT_PIN);
  }
  if (resetreas & NRF_POWER_RESETREAS_DOG_MASK)
  {
    reset_bits |= (1<<RESET_BIT_WDG);
  }
  if (resetreas & NRF_POWER_RESETREAS_SREQ_MASK)
  {
    reset_bits |= (1<<RESET_BIT_SOFT);
  }
  if (reset_bits == 0)
  {
    reset_bits |= (1<<RESET_BIT_ETC);
  }

#if CLI_USE(HW_RESET)
  cliAdd("reset", cliReset);
#endif

  return true;
}

void resetLog(void)
{
  const char *p_str[RESET_BIT_MAX] = {"POWER", "PIN", "WDG", "SOFT", "ETC"};

  for (int i=0; i<RESET_BIT_MAX; i++)
  {
    if (reset_bits & (1<<i))
    {
      printk("Reset : %s\n", p_str[i]);
    }
  }
}

void resetToBoot(void)
{
  // 부트로더를 BLE OTA DFU 모드로 진입시킨다.
  // USB로 받으려면 DFU_MAGIC_UF2_RESET / DFU_MAGIC_SERIAL_ONLY_RESET 사용.
  //
  nrf_power_gpregret_set(NRF_POWER, GPREGRET_DFU, DFU_MAGIC_OTA_RESET);

  sys_reboot(SYS_REBOOT_COLD);
}

void resetToReset(void)
{
  nrf_power_gpregret_set(NRF_POWER, GPREGRET_DFU, 0);

  sys_reboot(SYS_REBOOT_COLD);
}

uint32_t resetGetBits(void)
{
  return reset_bits;
}

void resetSetBits(uint32_t data)
{
  reset_bits = data;
}

void resetSetBootMode(uint32_t data)
{
  nrf_power_gpregret_set(NRF_POWER, GPREGRET_BOOT_MODE, (uint8_t)data);
}

uint32_t resetGetBootMode(void)
{
  return nrf_power_gpregret_get(NRF_POWER, GPREGRET_BOOT_MODE);
}


#if CLI_USE(HW_RESET)

void cliReset(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info"))
  {
    const char *p_str[RESET_BIT_MAX] = {"POWER", "PIN", "WDG", "SOFT", "ETC"};

    for (int i=0; i<RESET_BIT_MAX; i++)
    {
      if (reset_bits & (1<<i))
      {
        cliPrintf("Reset     : %s\n", p_str[i]);
      }
    }
    cliPrintf("Boot mode : 0x%02X\n", resetGetBootMode());
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "reset"))
  {
    cliPrintf("reset...\n");
    delay(100);
    resetToReset();
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "boot"))
  {
    cliPrintf("enter dfu...\n");
    delay(100);
    resetToBoot();
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("reset info\n");
    cliPrintf("reset reset\n");
    cliPrintf("reset boot\n");
  }
}

#endif


#endif
