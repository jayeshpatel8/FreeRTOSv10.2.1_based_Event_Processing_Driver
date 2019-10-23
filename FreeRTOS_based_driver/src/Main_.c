#include <string.h>
#include <Internal.h>
#include <Thread.h>
#include <Scheduler.h>
#include <Drv.h>
#include <Isr.h>
#include <Main.h>
/*****************************************************************************/
/* GLOBAL DATA                                                               */
/*****************************************************************************/

/*****************************************************************************/
/* LOCAL DATA                                                                */
/*****************************************************************************/
static BOOL main_event_hdlr(T_THREAD_EVENT * event);
static void thread_init(void);

static T_THREAD_CB thread_main_handlers[] = { main_event_hdlr,
                                              Scheduler_event_hdlr,
                                              NULL };

static FAST_MEM_DATA_SECTION T_THREAD main_thread_ = {
      .thread_name = "MAIN_THREAD",
      .thread_event_name = "MAIN_E",
      .event_handlers = thread_main_handlers,
      .thread_start = thread_init,
};

static T_THREAD *main_thread = &main_thread_;

DECLARE_SCHEDULER(main_scheduler, MAX_SCHEDULER_QUEUE_ENTRIES);

/*****************************************************************************/
/* LOCAL FUNCTIONS                                                           */
/*****************************************************************************/

#define TEST
#ifdef TEST

void Main_init(void);
void Main_reqSetMode(const t_base_cfg * P_MODE ,void (*cb)(void*),void * p_cb_data);
void Main_getState( void (*cb)(U32 State));
static int Main_init_done = 0;
/* A software timer that is started from the tick hook. */
static TimerHandle_t xTimer = NULL;
/* The rate at which data is sent to the queue.  The times are converted from
milliseconds to ticks using the pdMS_TO_TICKS() macro. */
#define mainTIMER_SEND_FREQUENCY_MS			pdMS_TO_TICKS( 2000UL )

/* Priorities at which the tasks are created. */
#define	main_TEST_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
/*-----------------------------------------------------------*/
void Test_cb1(void * str) {
	printf("Call Back CB1 - %s", (char *)str);
}
void Test_cb2(U32 State) {
	printf("Call Back CB2 - Get State : %d \n", State);
}
T_RESULT Test_cb3(void * str) {
	printf("Call Back CB3 - %s", (char *)str);
	return RESULT_OK;
}
static void TimerCallback(TimerHandle_t xTimerHandle)
{

	/* This is the software timer callback function.  The software timer has a
	period of two seconds and is reset each time a key is pressed.  This
	callback function will execute if the timer expires, which will only happen
	if a key is not pressed for two seconds. */

	/* Avoid compiler warnings resulting from the unused parameter. */
	(void)xTimerHandle;

	printf("Call Back - software timer\r\n");

	/* Send to the queue - causing the queue receive task to unblock and
	write out a message.  This function is called from the timer/daemon task, so
	must not block.  Hence the block time is set to 0. */
	if (Main_init_done)
		Test_simulate_SW_TIMER_interrupt_generation();
	// Re-start the Timer
	BaseType_t xReturned = xTimerStart(xTimer, 0);
}
/*-----------------------------------------------------------*/

static void testTask(void *pvParameters)
{

	/* Prevent the compiler warning about the unused parameter. */
	(void)pvParameters;

	t_base_cfg cfg = { .mode = ON };

	Main_reqSetMode(&cfg, Test_cb1, (void *)"PASSED: Drv Set Mode : ON \n");
	Main_getState(Test_cb2);

	cfg.mode = OFF;
	Main_reqSetMode(&cfg, Test_cb1, (void *)"PASSED: Drv Set Mode : OFF \n");
	Main_getState(Test_cb2);

	/* Scheduler Test */
	cfg.mode = ON;
	Main_reqSetMode(&cfg, Test_cb1, (void *)"PASSED: Drv Set Mode : ON \n");
	Main_getState(Test_cb2);

	if (FAILED(Scheduler_run(main_scheduler, Test_cb3, (void *)"PASSED: Scheduler_run 1\n")))
		printf("Failed : Scheduler_run 1\n");
	Scheduler_grant(main_scheduler, SCHEDULER_GRANT_1);
	if (FAILED(Scheduler_run(main_scheduler, Test_cb3, (void *)"PASSED: Scheduler_run 2\n")))
		printf("Failed : Scheduler_run 2\n");
	Scheduler_grant(main_scheduler, SCHEDULER_GRANT_1);
	if (FAILED(Scheduler_run_async(main_scheduler, Test_cb3, (void *)"PASSED: Scheduler_run_async 2\n")))
		printf("Failed : Scheduler_run_async 2\n");
	/* Grant is zero - Run shall not through */
	if (FAILED(Scheduler_run(main_scheduler, Test_cb3, (void *)"FAILED: Scheduler_run 3\n")))
		printf("Failed : Scheduler_run 3\n");
	if (FAILED(Scheduler_run_async(main_scheduler, Test_cb3, (void *)"FAILED: Scheduler_run_async 3\n")))
		printf("Failed : Scheduler_run_async 3\n");
	/* Scheduler Suspended case */
	Scheduler_suspend(main_scheduler);
	if (FAILED(Scheduler_run(main_scheduler, Test_cb3, (void *)"FAILED: Scheduler_run 4\n")))
		printf("Failed : Scheduler_run 4\n");
	if (FAILED(Scheduler_run_async(main_scheduler, Test_cb3, (void *)"FAILED: Scheduler_run_async 4\n")))
		printf("Failed : Scheduler_run_async 4\n");
	Scheduler_grant(main_scheduler, SCHEDULER_GRANT_1);

	printf("\n\nAll Test Completed ! \n\n\n\n");

}
/*-----------------------------------------------------------*/
/* Creates a Test Thread & SW Timer to simulate a Timer Interrupt
*/
void Main_TestInit() 
{
	const TickType_t xTimerPeriod = mainTIMER_SEND_FREQUENCY_MS;

	//Test Thread creation
	xTaskCreate(testTask, "TestThread", configMINIMAL_STACK_SIZE, NULL, main_TEST_TASK_PRIORITY, NULL);

	/* Create the software timer, but don't start it yet. */
	xTimer = xTimerCreate("Timer",				/* The text name assigned to the software timer - for debug only as it is not used by the kernel. */
		xTimerPeriod,		/* The period of the software timer in ticks. */
		pdFALSE,			/* xAutoReload is set to pdFALSE, so this is a one shot timer. */
		NULL,				/* The timer's ID is not used. */

		TimerCallback);/* The function executed when the timer expires. */

	//The scheduler has not started yet so a block time is not used.
	BaseType_t xReturned = xTimerStart(xTimer, 0);
}
int main_driver()
{
    Main_init();
	Main_TestInit();
    printf("Init Done \n");
	Main_init_done = 1;

    /* Main Thread Test */
	// Start the Thread & Timer
	vTaskStartScheduler();

	return 1;
}
#endif

/* This non-blocking-function posts HW CONF message to Thread.
 * and calls the call back once HW configuration is done */
static inline void Main_reqSetConfig(T_CFG cfg_type,
                                            const void * P_CFG,
                                            void (*cb)(void*),
                                            void * p_cb_data)
{
    T_EVENT_CFG event;
    event.cfg_type = cfg_type;
    event.cfg.P_CFG = P_CFG;
    event.completion_callback =cb;
    event.p_completion_callback_data =p_cb_data;
    /* thread_send_event_ex traps on fatal errors */
    Thread_send_event_ex(main_thread,
                                   THREAD_EVENT_SET_CFG, &event,
                                   sizeof(T_EVENT_CFG)
                                   ,THREAD_EVENT_SEND_OPTION_DO_NOT_OR);
}
static void Main_on_set_mode(T_EVENT_CFG set_mode )
{
    U32 new_mode = set_mode.cfg.P_MODE->mode;
    U32 old_mode = Drv_isActive();

    if (old_mode == new_mode)
        return;

    //mask all interrupts
    Isr_unmaskIrqs(false);

    LOG_EVENT(LOG_ON_SET_MODE,new_mode);

    if (!new_mode)
    {
        /*  Power OFF */
        Scheduler_suspend(main_scheduler);
    }

    // just forward the config to  DRV
    Drv_setMode(set_mode.cfg.P_MODE);

    if (new_mode)
    {
        /*  Power ON from OFF */
        if (!old_mode)
            Scheduler_init(main_scheduler, main_thread, SCHEDULER_GRANT_1);

        //unmask all interrupts if  is not OFF
        Isr_unmaskIrqs(true);
    }

    /* Call the completion call back */
    if(set_mode.completion_callback)
        (set_mode.completion_callback)(set_mode.p_completion_callback_data);
}

static void Main_on_set_config(const T_EVENT_CFG * P_SET_CFG )
{
    LOG_EVENT(MAIN_ON_SET_CONFIG, P_SET_CFG->cfg_type);
    switch (P_SET_CFG->cfg_type)
    {
        case CFG_SET_MODE:
            Main_on_set_mode(*P_SET_CFG);
        break;

        default:
            printf("HW CFG unknown CMD: %u", P_SET_CFG->cfg_type );
        break;
    }
}

void Main_on_get_state(T_GET_STATE_EVENT state_event)
{
    U32 state = OFF;
    switch(Drv_isActive())
    {
        case OFF:
            state = OFF;
            break;
        case ON:
            state = ON;
            break;
        default:
            state = OFF;
            break;
    }

    if(state_event.completion_callback)
        (state_event.completion_callback)(state);
}

/* Main  thread which handles the events and sends data to CPS
 */
static BOOL main_event_hdlr(T_THREAD_EVENT *event)
{
    BOOL status = TRUE;
    if (Drv_isActive())
    {
        LOG_EVENT(LOG_MAIN_EVENT_HANDLER_ENTER, event->event);
        switch (event->event)
        {
            case THREAD_EVENT_SET_CFG:
                Main_on_set_config(&event->parameters.cfg_event);
            break;
            case THREAD_GET_STATE:
                Main_on_get_state(event->parameters.state);
                break;
			case THREAD_EVENT_TIMEOUT:
#ifdef TEST
				printf("PASSED: Timer Timeout Event Processed : DRV ON \n");
#endif
				break;
            default:
                status = FALSE;
            break;
        }
        LOG_EVENT(LOG_MAIN_EVENT_HANDLER_FINISHED, event->event);
    }
    else
    {
        LOG_EVENT(LOG_MAIN_EVENT_HANDLER_ENTER_IN_OFF, event->event);
        switch (event->event)
        {
            case THREAD_EVENT_SET_CFG:
                Main_on_set_config(&event->parameters.cfg_event);
            break;
            case THREAD_GET_STATE:
				Main_on_get_state(event->parameters.state);

                break;
			case THREAD_EVENT_TIMEOUT:
#ifdef TEST
				printf("PASSED: Timer Timeout Event Processed : DRV OFF \n");
#endif
				break;
			default:
                status = FALSE;
            break;
        }
    }
    LOG_EVENT(LOG_MAIN_EVENT_HANDLER_EXIT, status);
    return status;
}

static void thread_init(void)
{
    Isr_init(main_thread);
}


/*****************************************************************************/
/* EXPORTED FUNCTIONS                                                        */
/*****************************************************************************/

/*  init function, called at boot ,
 */
void Main_init(void)
{
    T_RESULT res;

    res = Thread_create(main_thread);
    ASSERT(RESULT_OK, res, THREAD_CREATE);
}

void Main_reqSetMode(const t_base_cfg * P_MODE ,void (*cb)(void*),void * p_cb_data)
{
    Main_reqSetConfig(CFG_SET_MODE,P_MODE,cb,p_cb_data);
}

void Main_getState( void (*cb)(U32 State))
{
    T_GET_STATE_EVENT event;
    event.completion_callback = cb;

    /* thread_send_event_ex traps on fatal errors */
    Thread_send_event_ex(main_thread,
                                   THREAD_GET_STATE, &event,
                                   sizeof(T_GET_STATE_EVENT)
                                   ,THREAD_EVENT_SEND_OPTION_DO_NOT_OR);
}

