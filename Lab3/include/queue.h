#ifndef QUEUE
#define QUEUE

#define QUEUE_MAX_SIZE 2048

struct queue {  // circular queue
    int front;
    int rear;
    int size;
    char buf[QUEUE_MAX_SIZE];
};

/// Interrupt task queue
int task_queue_run(void);
int task_queue_add(int (*fn)(void), int);
int task_queue_preempt(void);

typedef struct task_Q {
  unsigned char used;
  // The handler function or call back functions.
  int (*fn)(void);
  // Priority, the 9 means the lowest priority.
  int priority;
  // Pointer to next recored.
  struct task_Q *next;
} task_q;
#endif

void queue_init(struct queue* q, int size);
int queue_empty(struct queue* q);
int queue_full(struct queue* q);
void queue_push(struct queue* q, char val);
char queue_pop(struct queue* q);
