
#include <stdlib.h>
#include <stdio.h>
typedef struct Queue {
    int front,end,size; //front,end are indicies for use in buf, total size
    unsigned int total_cap;
    char** urls;
}Queue;


Queue* create(unsigned int total_cap)
{
    Queue* q = (Queue*)malloc(sizeof(Queue));

    q->total_cap = total_cap;
    q->front = 0;
    q->size = 0;

    q->end = total_cap-1;
    q->urls = (char**)malloc(q->total_cap*sizeof(char*));
    return q;

}

int isFull(Queue* q)
{
    return (q->size == q->total_cap);
    
}


int isEmpty(Queue* q)
{

    return q->size == 0;
}

char* front(Queue* q)
{
    if(isEmpty(q))
    {
        printf("WARNING : EMPTY QUEUE \n");
        
    }
    return q->urls[q->front];
}

char* end(Queue* q)
{
    if(isEmpty(q))
    {
        printf("WARNING : EMPTY QUEUE \n");
        
    }
    return q->urls[q->end];
}


void enqueue(Queue* q, char* buf)
{
    if(isFull(q)) return;

    
    q->end = ((q->end+1)% q->total_cap);
    q->urls[q->end] = buf;
    q->size = q->size +1;


    //printf("ADDED TO QUEUE: %s\n", buf);
}

char* dequeue(Queue* q)
{
    if(isEmpty(q))
    {
        printf("WARNING : EMPTY QUEUE \n");
        return 0;
    }
    char* url = q->urls[q->front];
    q->front = (q->front+1)%(q->total_cap);
    q->size = q->size-1;
    return url;
}

void print_queue(Queue* q)
{
    for(int i = q->front%(q->total_cap); i<q->size; i++)
    {
        printf("INDEX:%d STRING:%s\n",i,q->urls[i]);
    }
}

void free_queue(Queue* q)
{
    for(int i = 0; i<q->end+1; i++)
    {
        free(q->urls[i]);
    }
    free(q->urls);
    free(q);
}