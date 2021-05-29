

#include<stdio.h>
#include<stdlib.h>
#include"malloc.h"
#include"queue.h"
#include <semaphore.h>  


void CreateQueue(PQUEUE Q,int maxsize){
    
	Q->pBase=(struct ElemType *)malloc(sizeof(struct ElemType)*maxsize);
    
	if(NULL==Q->pBase)
	{	    
	    MPCLOG(LLV_ERROR,"CreatQueue Failed Exit(1)\n");
		exit(-1);       
	}    
    memset(Q->pBase,0x00,sizeof(struct ElemType)*maxsize);
	Q->front=0;        
	Q->rear=0;
	Q->maxsize=maxsize;
    Q->quit=false;
    
    sem_init(&(Q->sem), 0, 1);

    pthread_mutex_init(&Q->queueMutex,NULL);    
    pthread_cond_init(&Q->queueCond,NULL);    
    
}

bool ResetElemFlagInQueue(PQUEUE Q,char * buffer,bool release)
{
    
    MPCLOG(LLV_INFO,"%s \n", __func__);
   
    sem_wait(&(Q->sem));
	int i=Q->front;
	if(Q->pBase!=NULL){
    	while(i%Q->maxsize!=Q->rear)
    	{
            if((Q->pBase+i)->bufferAddr == buffer){
                (Q->pBase+i)->release = release;
                sem_post(&(Q->sem));                 	
                return true;           
            }
        	i++;
    	}
	}
    sem_post(&(Q->sem)); 	
    
	return false;
}

bool FullQueue(PQUEUE Q)
{    
	if(Q->front==(Q->rear+1)%Q->maxsize)   
		return true;
	else
		return false;
}
bool EmptyQueue(PQUEUE Q)
{   
	if(Q->front==Q->rear)    
		return true;
	else
		return false;
}
bool Enqueue(int fdIndex,PQUEUE Q, struct ElemType * val)
{      
   
	sem_wait(&(Q->sem));
	if(Q->pBase==NULL||FullQueue(Q)||g_netStatus[fdIndex]==false){         
        sem_post(&(Q->sem)); 	
       	
        return false;
	}else{	   
	    
	    (Q->pBase+Q->rear)->fdIndex      = val->fdIndex;        
        (Q->pBase+Q->rear)->bufferAddr   = val->bufferAddr;        
        (Q->pBase+Q->rear)->release      = val->release;        
        (Q->pBase+Q->rear)->len          = val->len;        
        (Q->pBase+Q->rear)->justNotSend  = false;
        
		Q->rear=(Q->rear+1)%Q->maxsize;              
       
        sem_post(&(Q->sem));         
       
        pthread_mutex_lock(&Q->queueMutex);	        
       
	    pthread_cond_broadcast(&Q->queueCond);
	    pthread_mutex_unlock(&Q->queueMutex);        
        
        
		return true;
	}
}

static unsigned long freeReTransferCallNum = 0;
static unsigned long freeReTransferFindedNum = 0;
void DequeFreeReTransBuffer(PQUEUE Q,char * buffer){
    
    MPCLOG(LLV_INFO,"%s >>>>> on Q(%p) Q->sem=%ld,freeReTransferCallNum = %ld\n", __func__,Q,Q->sem,freeReTransferCallNum);
    //if(Q->pBase!=NULL){
        sem_wait(&(Q->sem));
        MPCLOG(LLV_INFO,"%s  On Q(%p) (len = %d) buffer=%p\n", __func__,Q,(Q->rear-Q->front+Q->maxsize) % Q->maxsize,buffer);

        freeReTransferCallNum++;
    	int i=Q->front;
        bool finded = false;
        MPCLOG(LLV_INFO,"%s  Q(%p) Q->sem=%ld,front =%d,rear = %d,freeReTransferCallNum = %ld,  freeReTransferFindedNum = %ld\n", __func__,Q,Q->sem,i,Q->rear,freeReTransferCallNum,freeReTransferFindedNum);

        if(Q->pBase!=NULL&&buffer!=NULL){
            while(i%Q->maxsize!=Q->rear)
        	{
        	    //MPCLOG(LLV_ERROR,"%s  i=%d,freeReTransferCallNum = %ld,  freeReTransferFindedNum = %ld\n", __func__,i,freeReTransferCallNum,freeReTransferFindedNum);
        	
                if((Q->pBase+i)->bufferAddr == buffer&&(Q->pBase+i)->justNotSend==false){
                    freeReTransferFindedNum++;
                    MPCLOG(LLV_INFO,"%s ReTransfer set  Flag on  Q(%p) Enqueue buffer=%p,i = %d, freeReTransferFindedNum = %ld\n", __func__,Q,buffer,i,freeReTransferFindedNum);

                    (Q->pBase+i)->justNotSend = true;  
                    finded = true;
                }
            	i++;
        	}
        }
        if(finded){
            MPCLOG(LLV_INFO,"%s  on Q(%p) to free buffer=%p\n", __func__,Q,buffer);
            free(buffer);
        }
        sem_post(&(Q->sem)); 	
    //}
    MPCLOG(LLV_INFO,"%s <<<<<<<<<<<<< on Q(%p) ,Q->sem=%ld\n", __func__,Q,Q->sem);

}

bool Dequeue(PQUEUE Q, struct ElemType *val)
{  
    MPCLOG(LLV_INFO,"%s >>>>>> on Q(%p) Enqueue Q->sem=%ld\n", __func__,Q,Q->sem);
    sem_wait(&(Q->sem));
	if(Q->pBase==NULL||EmptyQueue(Q))
	{	    
	    
	    sem_post(&(Q->sem));	
       
	    MPCLOG(LLV_INFO,"%s................................ failed,for queue is Empty \n", __func__);
   
		return false;
	}else{	    
		*val=Q->pBase[Q->front];
		Q->front=(Q->front+1)%Q->maxsize;
        MPCLOG(LLV_INFO,"%s(%p)................................ %d \n", __func__,Q,(Q->rear-Q->front+Q->maxsize) % Q->maxsize);
        sem_post(&(Q->sem));
        MPCLOG(LLV_INFO,"%s  <<<<<< on Q(%p) Q->sem=%ld\n", __func__,Q,Q->sem);
        		
		return true;
	}
}

void DestroyQueue(PQUEUE Q){
    sem_wait(&(Q->sem));
    
    ReleaseQueueBuffs(Q);
    sem_post(&(Q->sem));
    //sem_destroy(&(Q->sem));	
    pthread_mutex_destroy(&Q->queueMutex);
    pthread_cond_destroy(&Q->queueCond);
    
}

void ReleaseQueueBuffs(PQUEUE Q){

    int i=Q->front;
	if(Q->pBase!=NULL){
    	while(i%Q->maxsize!=Q->rear)
    	{
    	if((Q->pBase+i)->release == true){
                free((Q->pBase+i)->bufferAddr);        
            }
        	i++;
    	}
	}
    free(Q->pBase);
    Q->pBase=NULL;
}
