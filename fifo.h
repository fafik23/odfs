#ifndef ODFS_FIFO
#define ODFS_FIFO

#include <pthread.h>

typedef struct node_t{
	void *  data;
	struct node_t *prev;
	struct node_t *next;
} node_t;
 
typedef struct fifo_t{
	struct node_t *first;
	struct node_t *last;
	struct node_t *tmp_node;
	unsigned int len;
	pthread_mutex_t lock;
	pthread_cond_t cond;
} fifo_t;

int fifo_add(fifo_t *fifo,void * e);

int fifo_del(fifo_t *fifo,void * e);

int fifo_take(fifo_t *fifo,void ** e);

unsigned int fifo_len(struct fifo_t *fifo);

int fifo_init(struct fifo_t *fifo);

int fifo_wait(struct fifo_t *fifo);

int fifo_broadcast(struct fifo_t *fifo);

int fifo_exec(struct fifo_t *fifo,void (*exec)(void *,void *),void * arg );

int fifo_execn(struct fifo_t *fifo,void (*exec)(void *),void * arg,unsigned int n );

int fifo_free(struct fifo_t *fifo);

#endif
