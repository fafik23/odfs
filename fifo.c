/*! \file fifo.c
    \brief fifo code
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fifo.h"

#define DEBUG 1
#define DEBUG2 0

/**
* \fc fifo_add ...desc...
*
* (1) - error, (0) - ok
*/
int fifo_add(fifo_t *fifo,void * e){
	if(DEBUG2) { fprintf(stderr, "%s:%d debug: fifo_add: Start\n",__FILE__,__LINE__); }
	pthread_mutex_lock(&(fifo->lock));
	fifo->tmp_node = malloc(sizeof(struct node_t));
	if(fifo->tmp_node == NULL)
	{
		puts("Error in fifo add: Cannot Allocate Memory.");
		exit(1);
	}
	fifo->tmp_node->data = e;
	fifo->tmp_node->next = fifo->first;
	fifo->tmp_node->prev = NULL;

	fifo->first=fifo->tmp_node;
	
	if(fifo->len == 0) fifo->last =fifo->tmp_node;
	fifo->len++;
	pthread_mutex_unlock(&(fifo->lock));
	if(DEBUG2) { fprintf(stderr, "%s:%d debug: fifo_add: END\n",__FILE__,__LINE__); }
	return 0;
}

/**
* \fc fifo_del ...desc...
*
* (0) - ok
*/
int fifo_del(fifo_t *fifo,void * e){
	pthread_mutex_lock(&(fifo->lock));
	fifo->tmp_node=fifo->first;
	while (fifo->tmp_node) {
		if(fifo->tmp_node->data==e){
			if(DEBUG) { fprintf(stderr, "%s:%d debug: fifo_del: find data\n",__FILE__,__LINE__); }
			if(fifo->tmp_node->prev) fifo->tmp_node->prev->next=fifo->tmp_node->next;
			if(fifo->tmp_node->next) fifo->tmp_node->next->prev=fifo->tmp_node->prev;
			if(fifo->first==fifo->tmp_node) fifo->first=fifo->tmp_node->next;
			if(fifo->last==fifo->tmp_node) fifo->last=fifo->tmp_node->prev;
			fifo->len--;
			free(fifo->tmp_node);
			fifo->tmp_node=NULL;
			break;
		}
		fifo->tmp_node = fifo->tmp_node->next;
	}
	pthread_mutex_unlock(&(fifo->lock));
	return 0;
}

/**
* \fc fifo_take ...desc...
*
* (-1) - ... , (0) - ok
*/
int fifo_take(fifo_t *fifo,void ** e){
	pthread_mutex_lock(&(fifo->lock));
	fifo->tmp_node=fifo->first;
	if (fifo->len>0) {
		*e=fifo->first->data;
		fifo->first=fifo->first->next;
	} else {
		*e=NULL;
		return -1;
	}
	free(fifo->tmp_node);
	fifo->len--;
	pthread_mutex_unlock(&(fifo->lock));
	return 0;
}

/**
* \fc fifo_len ...desc...
*
* 
*/
unsigned int fifo_len(struct fifo_t *fifo){
	unsigned int len;
	pthread_mutex_lock(&(fifo->lock));
	len=fifo->len;
	pthread_mutex_unlock(&(fifo->lock));
	return len;
}

/**
* \fc fifo_init ...desc...
*
* (0) - ok
*/
int fifo_init(struct fifo_t *fifo) {
	if(DEBUG) { fprintf(stderr, "%s:%d debug: fifo_init: Start\n",__FILE__,__LINE__); }
	pthread_mutex_init(&(fifo->lock), NULL);
	pthread_cond_init (&(fifo->cond), NULL);
	pthread_mutex_lock(&(fifo->lock));
	fifo->first = NULL;
	fifo->last = NULL;
	fifo->tmp_node = NULL;
	fifo->len=0;
	pthread_mutex_unlock(&(fifo->lock));
	return 0;
}

int fifo_wait(struct fifo_t *fifo) {
	if(DEBUG) { fprintf(stderr, "%s:%d debug: fifo_wait: Start\n",__FILE__,__LINE__); }
	pthread_mutex_lock(&(fifo->lock));
	pthread_cond_wait(&(fifo->cond),&(fifo->lock));
	pthread_mutex_unlock(&(fifo->lock));
	return 0;
}

int fifo_broadcast(struct fifo_t *fifo) {
//	puts("fifo_broadcast: Start");
	pthread_mutex_lock(&(fifo->lock));
	pthread_cond_broadcast(&(fifo->cond));
	pthread_mutex_unlock(&(fifo->lock));
	return 0;
}

/**
* \fc fifo_exec ...desc...
*
* (0) - ok
*/
int fifo_exec(struct fifo_t *fifo,void (*exec)(void *,void *),void * arg ){
	if(DEBUG2) { fprintf(stderr, "%s:%d debug: fifo_exec: Start\n",__FILE__,__LINE__); }
	pthread_mutex_lock(&(fifo->lock));
	fifo->tmp_node=fifo->first;
	while (fifo->tmp_node) {
		exec(fifo->tmp_node->data,arg);
		fifo->tmp_node = fifo->tmp_node->next;
	}
	pthread_mutex_unlock(&(fifo->lock));
	if(DEBUG2) { fprintf(stderr, "%s:%d debug: fifo_exec: END\n",__FILE__,__LINE__); }
	return 0;
}

/**
* \fc fifo_execn ...desc...
*
* (0) - ok
*/
int fifo_execn(struct fifo_t *fifo,void (*exec)(void *),void * arg, unsigned int n ){
	unsigned int i=0;
	pthread_mutex_lock(&(fifo->lock));
	fifo->tmp_node=fifo->first;
	while (fifo->tmp_node && i<n) {
		exec(fifo->tmp_node->data);
		fifo->tmp_node = fifo->tmp_node->next;
		i++;
	}
	pthread_mutex_unlock(&(fifo->lock));
	return 0;
}

/**
* \fc fifo_free ...desc...
*
* (0) - ok
*/
int fifo_free(struct fifo_t *fifo){
	struct node_t *tmp;
	pthread_mutex_lock(&(fifo->lock));
	tmp=fifo->first;
	while (tmp) {
		fifo->tmp_node = tmp;
		tmp=tmp->next;
		free(fifo->tmp_node);
	}
	fifo->first = NULL;
	fifo->last = NULL;
	pthread_mutex_destroy(&(fifo->lock));
	pthread_cond_destroy(&(fifo->cond));
	return 0;
}

