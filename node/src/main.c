#include "zephyr.h"

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7


void node_test(void) {
    
}

K_THREAD_DEFINE(node_test_id, STACKSIZE, node_test, NULL, NULL, NULL, PRIORITY, 0, 0);

