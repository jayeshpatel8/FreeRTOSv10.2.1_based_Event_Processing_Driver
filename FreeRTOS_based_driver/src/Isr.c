/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "Drv.h"
#include "Isr.h"

/*****************************************************************************/
/* DEFINES                                                                   */
/*****************************************************************************/
/** \brief ISR function declaration and definition */
#define ISR_DEFINE(_name)                                                 \
  static void _name(U32 vector_number, void *p_param)

/*****************************************************************************/
/* TYPE DEFINES                                                              */
/*****************************************************************************/

/*****************************************************************************/
/* LOCAL DATA                                                                */
/*****************************************************************************/

/**
 * \brief  ISR context information
 */
struct
{
    OsIrqIsr isr_timeout;


    T_THREAD *worker_thread;

    /* ISR module initialized */
    BOOL initialized;
} isr;

/*****************************************************************************/
/* FUNCTION PROTOTYPES                                                       */
/*****************************************************************************/

/*****************************************************************************/
/* LOCAL FUNCTIONS                                                           */
/*****************************************************************************/
ISR_DEFINE(isr_timeout)
{
    hw.stat.irq_timeout++;

    /* thread_send_event traps on fatal errors */
    Thread_send_event(isr.worker_thread,
                         THREAD_EVENT_TIMEOUT,
                         THREAD_EVENT_SEND_OPTION_OR);
}

/*****************************************************************************/
/* EXPORTED FUNCTIONS                                                        */
/*****************************************************************************/
/**
 * \brief Install interrupt service routines and callbacks
 *
 * @return  None
 */
void Isr_init(T_THREAD *p_worker_thread)
{
    U32 rc;

    PTR_ASSERT(p_worker_thread, ISR_NO_THREAD_ERR);
    isr.worker_thread = p_worker_thread;

    /* Register IRQ handler*/
    rc = OsIrqCreate(
        &isr.isr_timeout,
        "TmoLisr", T_INTERRUPT_LINE_TIMEOUT,
        isr_timeout);
    if (rc != OS_SUCCESS)
        TRAP(ISR_TMO_ERR, rc);

    isr.initialized = TRUE;
}

/* Mask or unmaks the interrupts */
void Isr_unmaskIrqs( BOOL onOff )
{
    U32 rc = OS_SUCCESS;

    if( onOff )
    {
        /* isr enable */
        rc = OsIrqUnmask(&isr.isr_timeout);
        if (rc != OS_SUCCESS)
            TRAP(ISR_TMO_UNMASK, rc);
    }
    else
    {
        rc = OsIrqMask(&isr.isr_timeout);
        if (rc != OS_SUCCESS)
            TRAP(ISR_TMO_MASK, rc);
    }
}
void Test_simulate_SW_TIMER_interrupt_generation(void)
{
	isr_timeout(00, 00);
}