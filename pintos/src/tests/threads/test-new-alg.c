#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

#define count_TH 6

static thread_func test_new_alg_thread;
static struct semaphore sema;
void test_new_alg(void);


void test_new_alg(void){
ASSERT (!thread_mlfqs);

re_TYPE_alg ('F');
sema_init (&sema, 0);
thread_set_priority(PRI_MIN);
int  i;

for(i = 0; i<count_TH; i++){
char name[16];
snprintf (name, sizeof name, "Thread %d", i);
thread_create(name, i, test_new_alg_thread, &sema);
}

for(i = 0; i<count_TH; i++)
sema_up (&sema);


}


static void test_new_alg_thread (void *aux UNUSED){
  sema_down (&sema);

for(int i = 0; i<thread_current()->CPU_burst; i++){

printf ("%s iteration %d\n", thread_name (), i);
      //thread_yield ();
}

printf("%s with priority %d finished\n", thread_current()->name,thread_current()->priority);
}


