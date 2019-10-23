#if !defined(ISR_H)
#define ISR_H

/**
 @addtogroup MODULE NAME
 @{
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include <Thread.h>

/*****************************************************************************/
/* DEFINES                                                                   */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* EXPORTED FUNCTIONS                                                        */
/*****************************************************************************/
void Isr_init(T_THREAD *p_worker_thread);
void Isr_unmaskIrqs( BOOL onOff );

/*@}*/

#endif /* ISR_H */