
#ifndef __QUEUE_H_
#define __QUEUE_H_

#include <stdbool.h>
#include <semaphore.h>  

#include "plog.h"

#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<pthread.h>


#define QUEUE_MAXSIZE 1000*10

#pragma pack(push)
#pragma pack (1)

typedef struct ElemType{
    int fdIndex;
    char * bufferAddr;
    int len;
    bool release;
    bool justNotSend;
    
} ElemType;
#pragma   pack(pop)

typedef struct queue 
{    
	struct ElemType *pBase;
	int front;   
	int rear;    
	int maxsize; 
	
	bool quit;   
	sem_t sem;
    pthread_mutex_t queueMutex;
    pthread_cond_t  queueCond;

}QUEUE,*PQUEUE;
 
typedef enum
{  
    QueueFull = 0,  
    QueueEmpty,  
    QueueOK,  
    QueueFail  
} QueueStatus;
extern bool g_netStatus[3];

void CreateQueue(PQUEUE Q,int maxsize);
void TraverseQueue(PQUEUE Q);
bool FullQueue(PQUEUE Q);
bool EmptyQueue(PQUEUE Q);
bool Enqueue(int fdIndex,PQUEUE Q, struct ElemType *val);
bool Dequeue(PQUEUE Q, struct ElemType *val);
void DestroyQueue(PQUEUE Q);
void ReleaseQueueBuffs(PQUEUE Q);

#endif /* __QUEUE_H_ */
