#ifndef HW_DEF_H_
#define HW_DEF_H_


#include "bsp.h"


#define _DEF_FIRMWATRE_VERSION      "V241115R1"
#define _DEF_BOARD_NAME             "BARAM-QMK-H7S-FW"


#define _USE_HW_LED
#define      HW_LED_MAX_CH          4

#define _USE_HW_RESET

// CLI 는 Kconfig 의 APP_USE_CLI 로 켜고 끈다. (prj.conf / low_power.conf)
//
#ifdef CONFIG_APP_USE_CLI

#define _USE_HW_CDC

#define _USE_HW_UART
#define      HW_UART_MAX_CH         1
#define      HW_UART_CH_USB         _DEF_UART1
#define      HW_UART_CH_CLI         HW_UART_CH_USB

#define _USE_HW_CLI
#define      HW_CLI_CMD_LIST_MAX    32
#define      HW_CLI_CMD_NAME_MAX    16
#define      HW_CLI_LINE_HIS_MAX    8
#define      HW_CLI_LINE_BUF_MAX    64


//-- CLI
//
#define _USE_CLI_HW_LED             1
#define _USE_CLI_HW_UART            1
#define _USE_CLI_HW_RESET           1
#define _USE_CLI_HW_MODULE          1
#define _USE_CLI_HW_DISPLAY         1

#endif


//-- RTOS
//
#define _HW_DEF_RTOS_THREAD_PRI_CLI           5
#define _HW_DEF_RTOS_THREAD_PRI_DISPLAY       5

#define _HW_DEF_RTOS_THREAD_MEM_CLI           (4*1024)
#define _HW_DEF_RTOS_THREAD_MEM_DISPLAY       (1*1024)


#endif
