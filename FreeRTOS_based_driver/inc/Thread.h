/**
 * \file Thread.h
 * \brief Thread/Event queue functionality
 */

/**
 * @addtogroup Module Name
 * @{
 */
#ifndef THREAD_H
#define THREAD_H

#include "Internal.h"

/*******************************************************************
 *  MACRO DEFINITIONS
 ******************************************************************/

#define Thread_send_event(thread, event, option )        \
  (Thread_send_event_ex(thread, event, NULL, 0,option))
/*******************************************************************
 *  TYPE DEFINITIONS
 ******************************************************************/

typedef struct
{
    U32 bufferId; /**< Buffer to operate on */
} T_EVENT_BUFFER;

typedef enum
{
    CFG_SET_MODE=0,
    CFG_MAX
} T_CFG;
typedef struct
{
    union {
        const t_base_cfg *P_MODE;
        const void * P_CFG;
    }cfg;
    T_CFG cfg_type;
    void (*completion_callback)(void *);
    void * p_completion_callback_data;
} T_EVENT_CFG;

typedef struct
{
    void (*completion_callback)(void *p_data);
    void *p_data;
} T_EVENT_COMPLETION;

typedef struct
{
    void *scheduler;
    U32 tag;
} T_SCHEDULER_EVENT;

/**
 * \brief A list of all possible thread events
 */
typedef enum
{
    THREAD_EVENT_TIMEOUT,
    THREAD_EVENT_SET_CFG,
    THREAD_EVENT_SCHED_RUN,
    THREAD_EVENT_SCHED_GRANT,
    THREAD_CLOSE,
    THREAD_GET_STATE,
    THREAD_EVENT_MAX
} T_THREAD_EVENT_TYPE;

/**
 * \brief GET STATE
 */
typedef struct
{
    void (*completion_callback)(U32);
}T_GET_STATE_EVENT;

/**
 * \brief A list of all possible thread event send options
 */
typedef enum
{
    THREAD_EVENT_SEND_OPTION_DO_NOT_OR, /**< Always send */
    THREAD_EVENT_SEND_OPTION_OR,        /**< Send only if there is no event of same type queued already */
}T_THREAD_EVENT_SEND_OPTION;

/**
 * \brief Thread event
 */
typedef struct
{
    T_THREAD_EVENT_TYPE event; /**< event ID */

    /** \brief Union of all possible thread events */
    union
    {
        U32 data;  /**< Data */
        void *ptr; /**< data */
        /* Buffer related information */
        T_EVENT_BUFFER buffer_event;
        /* used by  events which flush the export buffer */
        T_EVENT_COMPLETION export_event;
        /* used by  scheduler events */
        T_SCHEDULER_EVENT scheduler_event;
        /*  HW Configuration */
        T_EVENT_CFG cfg_event;
        /*  State info */
        T_GET_STATE_EVENT state;
    } parameters;
} T_THREAD_EVENT;

/**
 * \brief Thread event information
 */
typedef struct
{
    T_THREAD_EVENT event; /**< event structure */
    BOOL processed;           /**< Indicates that the
                                 event has been processed. */
} T_THREAD_EVENT_ENTRY;

/**
 * \brief Thread event callback
 */
typedef BOOL (*T_THREAD_CB)(T_THREAD_EVENT *event);

typedef T_THREAD_CB *T_THREAD_CB_LIST;

typedef enum
{
    THREAD_STATE_INIT = 0, /**< Thread is in init state */
    THREAD_STATE_RUN,      /**< Thread is running */
    THREAD_STATE_SUSPEND   /**< Thread is suspended */
} T_THREAD_STATE;

typedef U16 T_THREAD_EVENT_INDEX;

/**
 * \brief Application Service Thread
 */
typedef struct
{
    /** Static information */

    /** \brief thread_start function is executed in the thread context
     * (NULL if not available) before the thread enters it's main loop */
    void (*thread_start)(void);

    /** \brief Event handler, NULL terminated array */
    T_THREAD_CB_LIST event_handlers;

    const char *thread_name;
    const char *thread_event_name;

    /** Runtime Information */

    T_THREAD_STATE state;    /**< Thread execution state */
    OsEvent event_id;         /**< Event ID used for task     */
    OsThread event_thread_id; /**< Thread id for event task   */
    /**< Flag used for checking T_THREAD_EVENT_TYPE already queued or not */
    BOOL thread_event_already_queued[THREAD_EVENT_MAX];

    spinlock_t event_lock; /**< Spinlock for event queue */

    /* circular thread event queue */
    T_THREAD_EVENT_INDEX thread_event_wr; /**< Writer location */
    T_THREAD_EVENT_INDEX thread_event_rd; /**< Reader location */
    T_THREAD_EVENT_ENTRY
    thread_event[MAX_THREAD_EVENT_ENTRIES]; /**< thread event queue */
} T_THREAD;

/*******************************************************************
 *  FUNCTION PROTOTYPES
 ******************************************************************/

T_RESULT Thread_create(T_THREAD *thread);
T_RESULT Thread_close(T_THREAD *thread);
T_RESULT Thread_send_event_ex(T_THREAD *thread,
                              T_THREAD_EVENT_TYPE event, void *data,
                              U32 size, T_THREAD_EVENT_SEND_OPTION option);

#endif /* THREAD_H */
/** @} */
