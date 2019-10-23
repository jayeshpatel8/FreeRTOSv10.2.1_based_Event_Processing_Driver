
#ifndef MAIN_H
#define MAIN_H

/**
 @addtogroup MAIN
 @{
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include <stdint.h>

/*****************************************************************************/
/* EXPORTED FUNCTIONS                                                        */
/*****************************************************************************/
void Main_init(void);
void Main_getState(void (*cb)(U32 State));
/*@}*/

#endif /* MAIN_H */
