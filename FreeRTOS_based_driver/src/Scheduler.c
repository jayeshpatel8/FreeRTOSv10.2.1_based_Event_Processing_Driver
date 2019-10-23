/**
 * \file scheduler.c
 * \brief scheduler
 */

/**
 * @addtogroup Module Name
 * @{
 */
#include <string.h>
#include "Scheduler.h"

/*******************************************************************
 *  LOCAL FUNCTIONS
 ******************************************************************/
static void scheduler_grant_incr_(T_SCHEDULER *scheduler,
                                      U32 grant)
{
    if (!scheduler)
        return;

    os_atomic_add_U32(&scheduler->current_grant, grant);
}

static void scheduler_grant_decr_(T_SCHEDULER *scheduler,
                                      U32 grant)
{
    if (!scheduler)
        return;

    os_atomic_sub_U32(&scheduler->current_grant, grant);
}

static T_RESULT scheduler_enqueue_(T_SCHEDULER *scheduler,
                                           T_SCHEDULER_CALLBACK func,
                                           void *func_args,
                                           T_RESULT *result,
                                           OsSem *sem)
{
    T_SCHEDULER_REMOTE_CALL *remote_call;
    T_RESULT local_result = RESULT_OK;

    U16 new_wr;

    if (!scheduler)
        return RESULT_PARAMETER_ERROR;

    os_spinlock_obtain(&scheduler->lock);

    /* check if queue is full */
    new_wr = (scheduler->queue_wr + 1) % scheduler->queue_length;
    if (new_wr == scheduler->queue_rd)
    {
        local_result = RESULT_NO_RESOURCES_AVAILABLE;
        goto exit;
    }

    remote_call = &scheduler->queue[scheduler->queue_wr];
    remote_call->processed = FALSE;
    remote_call->func = func;
    remote_call->func_args = func_args;
    remote_call->result = result;
    remote_call->sem = sem;

    /*
     * Make sure the info makes it to memory.  Make sure to do this BEFORE
     * queue_wr index is updated.
     */
    os_data_sync_barrier();

    scheduler->queue_wr = new_wr;
exit:
    os_spinlock_release(&scheduler->lock);

    return local_result;
}

static void scheduler_process_(T_SCHEDULER *scheduler)
{
    T_SCHEDULER_REMOTE_CALL *remote_call;
    T_RESULT call_result;
    U16 queue_rd;

    if (!scheduler || (scheduler->state != SCHEDULER_RUN))
        return;

    queue_rd = scheduler->queue_rd;
    while (scheduler->current_grant && (queue_rd != scheduler->queue_wr))
    {

        remote_call = &scheduler->queue[queue_rd];
        if (!remote_call->processed)
        {
            call_result = remote_call->func(remote_call->func_args);
            if (remote_call->result)
                *remote_call->result = call_result;

            /* Make sure all the memory operations are complete before
             * signalling completion. */
            os_data_sync_barrier();

            if (remote_call->sem)
                OsSemRelease(remote_call->sem);

            remote_call->processed = TRUE;
        }

        queue_rd = (queue_rd + 1) % scheduler->queue_length;
        scheduler->queue_rd = queue_rd;

        scheduler_grant_decr_(scheduler, SCHEDULER_GRANT_1);
    }
}

/*******************************************************************
 *  EXTERNAL FUNCTIONS
 ******************************************************************/
T_RESULT Scheduler_init(T_SCHEDULER *scheduler,
                                     T_THREAD *thread,
                                     U32 initial_grant)
{
    if (!scheduler || !thread)
        return RESULT_PARAMETER_ERROR;

    scheduler->thread = thread;
    scheduler->state = SCHEDULER_RUN;

    os_spinlock_init(&scheduler->lock);

    scheduler->current_grant = initial_grant;

    scheduler->queue_wr = 0;
    scheduler->queue_rd = 0;

    scheduler->initialized = TRUE;

    memset(scheduler->queue, 0x00,
           sizeof(T_SCHEDULER_REMOTE_CALL) * scheduler->queue_length);

    return RESULT_OK;
}

T_RESULT Scheduler_grant(T_SCHEDULER *scheduler, U32 grant)
{
    T_SCHEDULER_EVENT grant_event;
    T_THREAD *thread;

    if (!scheduler)
        return RESULT_PARAMETER_ERROR;

    thread = scheduler->thread;
    if (!thread)
        return RESULT_PARAMETER_ERROR;

    scheduler_grant_incr_(scheduler, grant);

    grant_event.scheduler = scheduler;
    grant_event.tag = 0;

    /* notify associated thread */
    return Thread_send_event_ex(thread, THREAD_EVENT_SCHED_GRANT,
                                    &grant_event, sizeof(grant_event),THREAD_EVENT_SEND_OPTION_DO_NOT_OR);
}

T_RESULT Scheduler_run_async(T_SCHEDULER *scheduler,
                                          T_SCHEDULER_CALLBACK func,
                                          void *func_args)
{
    T_THREAD *thread;
    T_SCHEDULER_EVENT run_event;

    if (!scheduler || !func)
        return RESULT_PARAMETER_ERROR;

    thread = scheduler->thread;
    if (!thread)
        return RESULT_PARAMETER_ERROR;

    scheduler_enqueue_(scheduler, func, func_args, NULL, NULL);

    run_event.scheduler = scheduler;
    run_event.tag = 0;

    /* notify associated thread */
    return Thread_send_event_ex(thread, THREAD_EVENT_SCHED_RUN,
                                    &run_event, sizeof(run_event),THREAD_EVENT_SEND_OPTION_DO_NOT_OR);
}

T_RESULT Scheduler_run(T_SCHEDULER *scheduler,
                                    T_SCHEDULER_CALLBACK func,
                                    void *func_args)
{
    T_THREAD *thread;
    T_RESULT result, local_result;
    T_SCHEDULER_EVENT run_event;
    OsSem sem;
    U32 rc;
    OsThread *current_thread;

    if (!scheduler || !func)
        return RESULT_PARAMETER_ERROR;

    thread = scheduler->thread;
    if (!thread)
        return RESULT_PARAMETER_ERROR;

    /*  Check if current context equals the scheduler thread
        and call func() directly.
     */
    rc = OsThreadGetCurrent(&current_thread);
    if (rc != OS_SUCCESS)
        return RESULT_WRONG_STATE;

    if (!current_thread)
        return RESULT_WRONG_CONTEXT;

    if (current_thread == &thread->event_thread_id)
        return RESULT_WRONG_CONTEXT;

    rc = OsSemCreate(&sem, "SCHEDULER_REMOTE_CALL", 0,
                        OS_SEM_TIMEOUT_SUPPORT);
    if (rc != OS_SUCCESS)
        return RESULT_NO_RESOURCES_AVAILABLE;

    /* enqueue remote procedure call */
    local_result =
        scheduler_enqueue_(scheduler, func, func_args, &result, &sem);
    if (FAILED(local_result))
    {
        result = local_result;
        goto exit;
    }

    run_event.scheduler = scheduler;
    run_event.tag = 0;

    /* notify associated thread */
    local_result = Thread_send_event_ex(thread,
                           THREAD_EVENT_SCHED_RUN,
                           &run_event, sizeof(run_event)
                           ,THREAD_EVENT_SEND_OPTION_DO_NOT_OR);
    if (FAILED(local_result))
    {
        result = local_result;
        goto exit;
    }

    /* wait until func() has been processed in the associated thread */
    OsSemObtain(&sem, OS_INFINITE, OS_INFINITE);

exit:
    /* delete semaphore */
    OsSemDelete(&sem);

    return result;
}

BOOL Scheduler_event_hdlr(T_THREAD_EVENT *event)
{
    switch (event->event)
    {
        case THREAD_EVENT_SCHED_GRANT:
        case THREAD_EVENT_SCHED_RUN:
            scheduler_process_(event->parameters.scheduler_event.scheduler);
        break;
        default:
        return FALSE;
    }

    return TRUE;
}
/* Suspend the scheduler when HW is turning off*/
void Scheduler_suspend(T_SCHEDULER *scheduler)
{
    if (!scheduler)
        return;

    scheduler->state = SCHEDULER_SUSPEND;
}

/** @} */
