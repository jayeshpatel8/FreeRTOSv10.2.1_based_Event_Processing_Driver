/**
 * \file scheduler.h
 * \brief Dispatch and schedule function callbacks to/in different processing
    environments.
 */

/**
 * @addtogroup Module Name
 * @{
 */
#ifndef scheduler_H_
#define scheduler_H_

#include "Internal.h"
#include "Thread.h"

/*******************************************************************
 *  MACRO DEFINITIONS
 ******************************************************************/
/**
 * \brief Declaration helper of T_SCHEDULER
 */
#define DECLARE_SCHEDULER(name_, queue_length_)                \
  static struct {                                                  \
    T_SCHEDULER base;                                          \
    T_SCHEDULER_REMOTE_CALL queue[queue_length_];              \
  } tmp_##name_ = { { #name_, queue_length_ } };               \
  static T_SCHEDULER *name_ = &tmp_##name_.base

/*******************************************************************
 *  TYPE DEFINITIONS
 ******************************************************************/
typedef enum
{
    SCHEDULER_RUN = 0,  /**< Scheduler is running */
    SCHEDULER_SUSPEND   /**< Scheduler is suspended */
} T_SCHEDULER_STATE;

typedef T_RESULT (*T_SCHEDULER_CALLBACK)(void *param);

typedef struct
{
    BOOL processed;
    T_SCHEDULER_CALLBACK func;
    void *func_args;
    T_RESULT *result;
    OsSem *sem;
} T_SCHEDULER_REMOTE_CALL;

/**
 * \brief Scheduler instance information
 */
typedef struct
{
    const char *scheduler_name;
    const U16 queue_length;

    /** Runtime Information */
    T_THREAD *thread;
    T_SCHEDULER_STATE state; /**< Scheduler running state */

    spinlock_t lock;             /**< Spinlock for object data */

    BOOL initialized;

    volatile U32 current_grant;

    /* circular work queue */
    volatile U16 queue_wr; /**< Writer location */
    volatile U16 queue_rd; /**< Reader location */

    T_SCHEDULER_REMOTE_CALL queue[1];
} T_SCHEDULER;

typedef enum
{
    SCHEDULER_GRANT_NONE = 0,  /**< Scheduler has no grant */
    SCHEDULER_GRANT_1          /**< Scheduler grant 1 */
} T_SCHEDULER_GRANT;

/*******************************************************************
 *  FUNCTION PROTOTYPES
 ******************************************************************/

T_RESULT Scheduler_init(T_SCHEDULER *scheduler,
                                     T_THREAD *thread,
                                     U32 initial_grant);
T_RESULT Scheduler_grant(T_SCHEDULER *scheduler, U32 grant);
T_RESULT Scheduler_run_async(T_SCHEDULER *scheduler,
                                          T_SCHEDULER_CALLBACK func,
                                          void *func_arg);
T_RESULT Scheduler_run(T_SCHEDULER *scheduler,
                                    T_SCHEDULER_CALLBACK func,
                                    void *func_args);
BOOL Scheduler_event_hdlr(T_THREAD_EVENT *event);
void Scheduler_suspend(T_SCHEDULER *scheduler);

#endif /* scheduler_H_ */
/** @} */
