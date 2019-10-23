#if !defined(INTERNAL_H)
#define INTERNAL_H

/**
 @addtogroup Module Name>
 @{
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include <extern.h>
/*****************************************************************************/
/* DEFINES                                                                   */
/*****************************************************************************/
/** \brief Structure tag name for debugging */
#define TAG_NAME "__MODULE_NAME__"

/**
 * \brief Macro helper for trap
 */
#define FATAL(_err_value, _static_id)  printf(" FATAL %s %s %ld \n" ,#_static_id ,__FILE__,__LINE__)

/**
 * \brief A test for conditions that are always expected to be TRUE.
 */
#define ASSERT(_expected, _result, _static_id) if (_expected != _result){printf(" ASSERT %s %s %ld \n" ,#_static_id ,__FILE__,__LINE__);}

/**
 * \brief Define an ASSERT macro for checking for a NULL pointer.
 */
#define PTR_ASSERT(_ptr, _static_id) \
  if ((_ptr) == NULL) {                                                        \
    FATAL(0, _static_id);                                              \
  }


#define TRAP(a,b)

#if defined(FAILED)
#undef FAILED
#define FAILED(result_) ((int)result_ >= (int)RESULT_FAILURE)
#endif

#if defined(SUCCEEDED)
#undef SUCCEEDED
#define SUCCEEDED(result_) ((int)result_ < (int)RESULT_FAILURE)
#endif

#define MAX_THREAD_EVENT_ENTRIES 15
#define MAX_SCHEDULER_QUEUE_ENTRIES 15

#define LOG_EVENT(a,b) //printf(#a" - \n");
#define log_event(a,b) //printf(#a" - \n");
/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/

/**
 * @typedef T_Result
 * \brief Enumeration of common  return results
 */
typedef enum {
  RESULT_OK,
  RESULT_TRUE,
  RESULT_FALSE,

  RESULT_FAILURE = 1000,
  RESULT_NOT_HANDLED,
  RESULT_NOT_SUPPORTED,
  RESULT_NOT_IMPLEMENTED,
  RESULT_PARAMETER_ERROR,
  RESULT_NO_RESOURCES_AVAILABLE,
  RESULT_WRONG_STATE,
  RESULT_WRONG_CONFIGURATION,
  RESULT_WRONG_CONTEXT,
} T_RESULT;

/**
 * \brief Structure of globals
 */
typedef struct {
  /* Signature to identify this structure in a memory dump */
  U8 tag_name[sizeof(TAG_NAME)];

  /* TRUE=initialized, FALSE=NOT initialized */
  BOOL initialized;

  struct {
    /* Count total number of export timeout IRQs */
    U32 irq_timeout;
  } stat;

  /* Global structure with basic HW cfg params */
  t_base_cfg config;
} t_HW;

typedef enum {
  LOG_ON_TIMER_EXPIRED,
  LOG_ON_SET_MODE,
  THREAD_EVENT_FUNC_START,
  THREAD_EVENT_FUNC_EVENT_RECEIVED,
  THREAD_EVENT_FUNC_TO_BE_PROCESSED,
  THREAD_EVENT_FUNC_START_PROCESSING,
  THREAD_EVENT_FUNC_PROCESSED,
  MAIN_ON_SET_CONFIG,
  LOG_LAST_EVENT
} T_LOGEVENT;


#define LOG_ENTRY_SIZE 5

typedef struct {
    U32 timestamp;
    T_LOGEVENT event;
    U32 eventData;
    U32 stat;
}t_log_entry;


/*****************************************************************************/
/* GLOBAL DATA                                                               */
/*****************************************************************************/
extern volatile t_HW hw;

/*@}*/

#endif /* INTERNAL_H */
