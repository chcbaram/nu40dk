#include "module.h"




typedef struct
{
  int32_t   count;
  module_t *p_module;
} module_info_t;


static bool moduleBegin(void);
#if CLI_USE(HW_MODULE)
static void cliModule(cli_args_t *args);
#endif


static module_info_t info;

extern uint32_t _smodule;
extern uint32_t _emodule;




bool moduleInit(void)
{
  bool ret;


  info.count    = ((int)&_emodule - (int)&_smodule)/sizeof(module_t);
  info.p_module = (module_t *)&_smodule;

  logPrintf("[  ] moduleInit()\n");
  logPrintf("     count : %d\n", info.count);

  ret = moduleBegin();

#if CLI_USE(HW_MODULE)
  cliAdd("module", cliModule);
#endif

  return ret;
}

bool moduleBegin(void)
{
  bool ret = true;


  for (int pri = MODULE_PRI_HIGH; pri <= MODULE_PRI_LOW; pri++)
  {
    for (int i=0; i<info.count; i++)
    {
      if (info.p_module[i].priority < MODULE_PRI_HIGH ||
          info.p_module[i].priority > MODULE_PRI_LOW)
      {
        if (pri == MODULE_PRI_HIGH)
        {
          logPrintf("     %s priority %d Fail\n",
                    info.p_module[i].name, info.p_module[i].priority);
          ret = false;
        }
        continue;
      }

      if (info.p_module[i].priority == pri && info.p_module[i].init != NULL)
      {
        bool mod_ret;

        mod_ret = info.p_module[i].init();
        ret &= mod_ret;

        logPrintf("     %-16s %s\n", info.p_module[i].name, mod_ret ? "OK":"Fail");
      }
    }
  }

  return ret;
}


#if CLI_USE(HW_MODULE)

void cliModule(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info"))
  {
    cliPrintf("count : %d\n", info.count);

    for (int i=0; i<info.count; i++)
    {
      cliPrintf("%d : %-16s pri %d\n",
                i,
                info.p_module[i].name,
                info.p_module[i].priority);
    }
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("module info\n");
  }
}

#endif
