#include "thread.h"
#include "mem.h"
#include "syscall.h"
#include "terminal.h"
#include "time.h"
#include "uart.h"
// Functions in switch.S
extern void switch_to(Thread *, Thread *);
extern Thread *get_current(void);

//=======================================================
// thread queues
extern Thread_q running = {NULL, NULL};
extern Thread_q waiting = {NULL, NULL};
extern Thread_q deleted = {NULL, NULL};
static thread_count = 0;
static Thread *startup;
// int a;

//========================================================
// Some private functions

void thread_init() {
  startup = pmalloc(1);
  startup->id = thread_count++;
  running.begin = NULL;
  running.end = NULL;
  waiting.begin = NULL;
  waiting.end = NULL;
  deleted.begin = NULL;
  deleted.end = NULL;
  asm volatile("msr tpidr_el1, %[startup];" ::[startup] "r"(startup));
  return;
}
// Insert in begin
void thread_q_add(Thread_q *Q, Thread *t) {
  t->prev = NULL;
  t->next = Q->begin;
  if (Q->begin != NULL)
    Q->begin->prev = t;
  Q->begin = t;
  if (Q->end == NULL) {
    Q->end = t;
  }
  return;
}
// Pop from end of Q
Thread *thread_q_pop(Thread_q *Q) {
  Thread *ret = Q->end;
  if (ret != NULL) {
    // Renew end of queue
    Q->end = ret->prev;
    if (Q->begin == ret) {
      // may be NULL
      Q->begin = ret->prev;
    }
    ret->prev = NULL;
    ret->next = NULL;
  }
  return ret;
}

// FIXME: Use a better algorithm
Thread *thread_q_delete(Thread_q *Q, Thread *target) {
  Thread *t = NULL;
  Thread *s = Q->begin;
  t = thread_q_pop(Q);
  // Iterative search from end of queue
  while (t != target && t != NULL) {
    thread_q_add(Q, t);
    t = thread_q_pop(Q);
    // Stop condition, start of the queue
    if (t == s && t != target) {
      thread_q_add(Q, t);
      return NULL;
    }
  }
  return t;
}
// Delete by ID
Thread *thread_q_delete_id(Thread_q *Q, int id) {
  Thread *t = NULL;
  Thread *s = Q->begin;
  t = thread_q_pop(Q);
  while (t->id != id && t != NULL) {
    thread_q_add(Q, t);
    t = thread_q_pop(Q);
    if (t == s && t->id != id) {
      thread_q_add(Q, t);
      // uart_puts("\nDelete by id fail: ");
      return NULL;
    }
  }
  return t;
}

/************************************************************************
 * Create thread with a separate kernel SP and user SP.
 *
 * @fn: The first function will be execute after thread created.
 ***********************************************************************/
Thread *thread_create(void (*fn)(void *)) {
  Thread *cur = pmalloc(0); // Get the small size
  cur->child = 0;
  cur->handler = NULL;
  cur->regs.lr = fn;
  cur->regs.sp = ((char *)cur) + 0x1000 - 16; // The stack will grow lower ???
  cur->regs.fp = ((char *)cur) + 0x1000 - 16; // No matter?
  cur->id = thread_count++;                   // Set ID
  cur->status = run;                          // Set the status
  cur->sp_el0 = pmalloc(0) + 0x1000 - 16;     // Separate from kernel stack
  thread_q_add(&running, cur);                // Add to thread queue
  uart_puts("thread create! ");
  uart_puts("cur->id: ");
  uart_puti(cur->id);
  uart_puts("\n\t cur: From ");
  uart_puth((char *)cur);
  uart_puts(" To ");
  uart_puth((char *)cur+0x1000);
  uart_puts("\n\t cur->regs.sp(kernel stack): From ");
  uart_puth(cur->regs.sp+16);
  uart_puts(" To ");
  uart_puth(cur->regs.sp+16-0x1000);
  uart_puts("\n\t cur->sp_el0(user stack): From ");
  uart_puth(cur->sp_el0+16);
  uart_puts(" To ");
  uart_puth(cur->sp_el0+16-0x1000);
  uart_puts("\n");
  // print_queue();
  // a = 10;
  return cur;
}

void idle(void) {
  while (1) {
    // uart_puts("idle()\n");
    kill_zombies();
    schedule();
  }
  return;
}

/*************************************************************************
 * Find if there exist the child is still running but parent in delete queue is deleted
 *************************************************************************/
void kill_zombies(void) {
  Thread *t = thread_q_pop(&deleted);
  while (t != NULL) {
    // uart_puts("\nkill zombies: ");
    // uart_puti(t->child);
    if (t->child > t->id)
      sys_kill(t->child);
    pfree(t->sp_el0);
    pfree(t);
    t = thread_q_pop(&deleted);
  }
  return;
}

/************************************************************************
 * Switch to another thread in running queue
 *
 * If no availiable thread in running queue, just create a terminal therad
 * and run.
 *************************************************************************/
void schedule() {
  // Test if thread add to queue successfully
  // uart_puts("--Scheduling-1--\n");
  // print_queue();

  // reschedule the thread order
  Thread *t = thread_q_pop(&running);
  thread_q_add(&running, t);

  // Test if thread add to queue successfully
  // uart_puts("---Scheduling-2---\n");
  // print_queue();

  // RR
  if (t == NULL) {
    uart_puts("create terminal thread!\n");
    terminal_run_thread();
    idle();
  }
  // if running thread is done then delete this thread
  if (t == running.begin && t == running.end) {
    if (t->status == wait) {
      thread_q_add(&deleted, thread_q_pop(&running));
      terminal_run_thread();
      t = thread_q_pop(&running);
      thread_q_add(&running, t);
    }
    // sys_kill(t->id);
    // idle();
  }
  // Test if thread add to queue successfully
  // uart_puts("---Scheduling-3---\n");
  // print_queue();

  Thread *cur = get_current();

  if (cur != NULL) {
    // uart_puts("From id ");
    // uart_puti(cur->id);
    // uart_puts(" ");
    // uart_puthl(cur);
    // uart_puts(" To id ");
    // uart_puti(t->id);
    // uart_puts(" ");
    // uart_puthl(t);
    // uart_puts("\n");
    switch_to(cur, t);
    // uint64_t stack; // Which contain the exception and value
    // asm volatile("mov  %[stack], sp;" : [stack] "=r"(stack) :);
    // uart_puts("stack\n");
    // uart_puth(stack);
    // uart_puts("\n");
  } else {
    uart_puts("initial switch\n");
    switch_to(startup, t);
  }
  return; // This return may never used
}

/***********************************************************************
 * Move current thread from running queue to deleted queue
 **********************************************************************/
void exit() {
  Thread *t = get_current();
  thread_q_delete(&running, t);
  thread_q_add(&deleted, t);
  uart_puts("[exit] ");
  uart_puti(t->id);
  uart_puts("\n");
  schedule();
  return;
}

/************************************************************************
 * Test foo()
 ***********************************************************************/
void foo(void *a) {
  Thread *t = get_current();
  for (int i = 0; i < 10; i++) { //i<10
    uart_puts("Thread id:");
    uart_puti(t->id);
    uart_puts(", iter: ");
    uart_puti(i);
    uart_puts("\n");
    delay(1000000);
    schedule();
  }
  exit();
  return;
}

/*************************************************************************
 * Test function
 ************************************************************************/
void test_thread_queue(void) {
  asm volatile("msr	tpidr_el1,	%[startup];" ::[startup] "r"(startup));
  for (int i = 0; i < 5; i++) {
    uart_puts("==========================\n");
    thread_create(foo);
  }
  idle();
  return;
}

/*************************************************************************
 * Wrapper function of the thread create fo terminal_run()
 ************************************************************************/
void terminal_run_thread() {
  thread_create(terminal_run);
  return;
}

void print_queue(){
  // Test if thread add to queue successfully
  Thread *cur= running.begin;
  int count=0;
  uart_puts("Queue address: ");
  uart_puth(&running);
  uart_puts("\n");
  // uart_puti(a);
  // a= a+1;
  uart_puts("run ID:");
  if (running.begin==running.end){
    uart_puti(cur->id);
    uart_puts(" ");
  }
  for (;cur!=running.end;cur=cur->next){
      count++;
      uart_puti(cur->id);
      uart_puts(" ");
  }
  uart_puti(cur->id);
  uart_puts(" ");
  uart_puts("\n");
  // // Test if thread add to queue successfully
  // cur= deleted.begin;
  // count=0;
  // uart_puts("delete ID:");
  // for (;cur!=deleted.end;cur=cur->next){
  //     count++;
  //     uart_puti(cur->id);
  //     uart_puts(" ");
  // }
  // if (deleted.begin==deleted.end){
  //   uart_puti(cur->id);
  // }
  // uart_puts("\n");
  //   // Test if thread add to queue successfully
  // cur= waiting.begin;
  // count=0;
  // uart_puts("waiting ID:");
  // for (;cur!=waiting.end;cur=cur->next){
  //     count++;
  //     uart_puti(cur->id);
  //     uart_puts(" ");
  // }
  // if (waiting.begin==waiting.end){
  //   uart_puti(cur->id);
  // }
  // uart_puts("\n-----------\n");
}
