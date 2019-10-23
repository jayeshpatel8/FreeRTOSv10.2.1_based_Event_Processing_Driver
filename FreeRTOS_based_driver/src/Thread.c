/**
 * \file Thread.c
 * \brief  thread functions
 */

/**
 * @addtogroup Module Name
 * @{
 */

#include <string.h>
#include "Thread.h"
#include "Internal.h"

/*****************************************************************************/
/* LOCAL FUNCTIONS                                                           */
/*****************************************************************************/
static void thread_event_func(void *param) {

    T_THREAD *thread = (T_THREAD *)param;
    T_THREAD_EVENT_INDEX thread_event_rd;
    if (!thread)
        return;

    /* call the thread start function (if needed) in the thread context */
    if (thread->thread_start)
        thread->thread_start();

    thread->state = THREAD_STATE_RUN;

    do
    {
        log_event(thread_event_func_START, 0);
        S32 res = (S32)OsEventWait(
            &thread->event_id, OS_INFINITE, OS_INFINITE);
        ASSERT(OS_SUCCESS, res, EVENT_WAIT);

        log_event(thread_event_func_EVENT_RECEIVED, 0);

        thread_event_rd = thread->thread_event_rd;
        while (thread_event_rd != thread->thread_event_wr && THREAD_STATE_RUN == thread->state)
        {
            T_THREAD_EVENT_ENTRY *event_entry = &thread->thread_event[thread_event_rd];
            thread->thread_event_already_queued[event_entry->event.event] = FALSE;
            /* Make sure all the memory operations are complete before
             * accessing the event. */
            log_event(thread_event_func_TO_BE_PROCESSED, thread_event_rd);
            os_data_sync_barrier();

            if (!event_entry->processed)
            {
                log_event(thread_event_func_START_PROCESSING, thread_event_rd);
                /* run through all event handlers*/
                T_THREAD_CB_LIST hdlr = thread->event_handlers;
                while (hdlr && !event_entry->processed)
                {
                    if (*hdlr)
                    {
                        event_entry->processed = (*hdlr)(&event_entry->event);
                        hdlr++;
                    }
                    else
                    {
                        log_event(thread_event_func_NOT_PROCESSED, event_entry->processed);
                        break;
                    }
                }
            }

            log_event(thread_event_func_PROCESSED, event_entry->processed);
            thread_event_rd = (thread_event_rd + 1) % MAX_THREAD_EVENT_ENTRIES;
            thread->thread_event_rd = thread_event_rd;
        }
    } while (THREAD_STATE_RUN == thread->state);
}

/*****************************************************************************/
/* EXPORTED FUNCTIONS                                                        */
/*****************************************************************************/
T_RESULT Thread_create(T_THREAD *thread)
{
    if (!thread || !thread->thread_name || !thread->thread_event_name ||
           !thread->event_handlers)
          return RESULT_PARAMETER_ERROR;

    /* create thread event */
    if (OsEventCreate(&thread->event_id, thread->thread_event_name,
                         OS_EVENT_AUTO_RESET) != OS_SUCCESS)
        return RESULT_NO_RESOURCES_AVAILABLE;

    os_spinlock_init(&thread->event_lock);

    thread->thread_event_wr = 0;
    thread->thread_event_rd = 0;

    /* create thread */
    thread->state = THREAD_STATE_INIT;
    if (OsThreadCreate(
            &thread->event_thread_id, thread->thread_name,
            thread_event_func, thread) != OS_SUCCESS)
        return RESULT_NO_RESOURCES_AVAILABLE;

    /* start thread */
    if (OsThreadStart(&thread->event_thread_id) != OS_SUCCESS)
        return RESULT_FAILURE;

    return RESULT_OK;
}

T_RESULT Thread_close(T_THREAD *thread)
{
    return RESULT_NOT_SUPPORTED;
}
/**
 *  if option = THREAD_EVENT_SEND_OPTION_OR case ,it will only post
 *  this event_type if it's' not present in Thread event Queue.
 */
T_RESULT Thread_send_event_ex(T_THREAD *thread,
                                            T_THREAD_EVENT_TYPE event, void *data,
                                            U32 size, T_THREAD_EVENT_SEND_OPTION option)
{
    T_THREAD_EVENT_ENTRY *event_entry;
    T_THREAD_EVENT_INDEX thread_event_rd;

    if (!thread)
        return RESULT_PARAMETER_ERROR;

    if (thread->state != THREAD_STATE_RUN)
        return RESULT_NOT_HANDLED;

    os_spinlock_obtain(&thread->event_lock);

    if(THREAD_EVENT_SEND_OPTION_OR == option)
    {
        if(TRUE == thread->thread_event_already_queued[event])
        {
            os_spinlock_release(&thread->event_lock);
            return RESULT_OK;
        }
        else
        {
            thread->thread_event_already_queued[event] = TRUE;
        }
    }
    event_entry = &thread->thread_event[thread->thread_event_wr];
    event_entry->processed = FALSE;
    event_entry->event.event = event;

    if (data)
    {
        if (size > sizeof(event_entry->event.parameters))
        {
            os_spinlock_release(&thread->event_lock);
            return RESULT_PARAMETER_ERROR;
        }
        memcpy_s(&event_entry->event.parameters, size, data, size);
    }

    /*
     * Make sure the info makes it to main memory.  Make sure to do this BEFORE
     * the thread_event_wr index is updated.
     */
    os_data_sync_barrier();

    /*
     * Get read index before updating write index in case
     * read thread processes event and updates rd index
     * before reaching overflow check
     */
    thread_event_rd = thread->thread_event_rd;
    thread->thread_event_wr = (thread->thread_event_wr + 1) % MAX_THREAD_EVENT_ENTRIES;

    /* Check for overflow */
    if (thread->thread_event_wr == thread_event_rd)
    {
        FATAL(RESULT_FAILURE, THREAD_EVENT_QUEUE_OVERRUN);
    }

    os_spinlock_release(&thread->event_lock);

    S32 res = (S32)OsEventSet(&thread->event_id);
    ASSERT(OS_SUCCESS, res, THREAD_EVENT_NOT_SET);

    return RESULT_OK;
}

/** @} */
