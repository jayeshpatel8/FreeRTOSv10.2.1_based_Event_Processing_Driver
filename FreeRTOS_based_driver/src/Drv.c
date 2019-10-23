

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include <string.h>
#include "Drv.h"
#include <Pow.h>
#include "Isr.h"
#include "Internal.h"

/*****************************************************************************/
/* DEFINES                                                                   */
/*****************************************************************************/
/*****************************************************************************/
/* LOCAL DATA                                                                */
/*****************************************************************************/
volatile t_HW hw;
/*****************************************************************************/
/* LOCAL FUNCTIONS                                                           */
/*****************************************************************************/
static BOOL Drv_isHWStatusActive(void)
{
  return FALSE;
}

/*****************************************************************************/
/* EXPORTED FUNCTIONS                                                           */
/*****************************************************************************/

/* Configure the HW */
void Drv_setMode( const t_base_cfg * P_CFG )
{
    //if new mode is OFF
    if(!P_CFG->mode)
    {
        //if current HW is not OFF
        if(hw.config.mode) // switching on --> off
        {
            BOOL result;

            result = Drv_isHWStatusActive();
            ASSERT(OS_FALSE,result,POWER_OFF_REQUEST_ACTIVE);
        }
    }
    else
    {
        //If current mode is OFF
        if(!hw.config.mode)
        {
            //enable power and clock for HW
            Pow_setPowCfg(hw.config.mode,1);
            //enable module
        }
    }
    hw.config.mode = P_CFG->mode;
}

BOOL Drv_isActive(void)
{
    return hw.config.mode;
}
