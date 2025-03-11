#include "zephyr.h"

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

void gateway_test(void) {
    
}

K_THREAD_DEFINE(gateway_test_id, STACKSIZE, gateway_test, NULL, NULL, NULL, PRIORITY, 0, 0);