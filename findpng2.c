#define _GNU_SOURCE
#include <search.h>
#include "curl_xml_edit.c"


void DEBUG_PRINT_RECV_BUF(RECV_BUF * p_recv_buf)
{
    printf("\nPRINTING BUFFER: ");
    
    for(int i = 0; i<p_recv_buf->max_size; i++)
    {
        printf("%c",p_recv_buf->buf[i]);
    }

    printf("\n");
}
void DEBUG_VALIDATE_HASH_TABLE()
{
    char data[] = "http://ece252-1.uwaterloo.ca:2531/image?img=1&part=0"; 
    ENTRY test;
    test.key = data;
    ENTRY * test_p;
    printf("0 NOT FOUND, 1 IF FOUND, RESULT: %d\n",hsearch_r(test, FIND, &test_p, hash_data)); 
}

int get_local_total(){
    pthread_mutex_lock(&mutex);
    int temp = png_total;
    pthread_mutex_unlock(&mutex);
    return temp;
}
int queue_size(){
    pthread_mutex_lock(&mutex);
    int temp = q->size;
    pthread_mutex_unlock(&mutex);
    return temp;
}

void increase_active_threads(){
    pthread_mutex_lock(&mutex);
    active_threads++;
    pthread_mutex_unlock(&mutex);
    return;
}

void decrease_active_threads(){
    pthread_mutex_lock(&mutex);
    active_threads--;
    pthread_mutex_unlock(&mutex);
    return;
}

int get_active_threads(){
    pthread_mutex_lock(&mutex);
    int temp = active_threads;
    pthread_mutex_unlock(&mutex);
    return temp;
}
void thread_cleanup(void *arg)
{
    int is_locked = pthread_mutex_trylock(&mutex);
    if(is_locked == 0)
    {
        pthread_mutex_unlock(&mutex);
    }

}

void *dobby(void *arg)
{
    sem_wait(&complete);
    

    for (int i = 0; i < t; i++)
    {
        pthread_cancel(p_tids[i]);
        pthread_join(p_tids[i], NULL);
    }
 
    
    
    return 0;
}


void* get_pngs(void *data)
{
    CURL *curl_handle;
    CURLcode res;
    char *url = data;
    RECV_BUF recv_buf;
    int done = 0; 
    pthread_cleanup_push(thread_cleanup, NULL);
    while (!done)
    {
        int local_total = get_local_total();
        if (local_total >= n){
            done = 1;
            sem_post(&complete);
            break;
            
        }
        increase_active_threads();
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl_handle = easy_handle_init(&recv_buf, url);

        if (curl_handle == NULL)
        {
            fprintf(stderr, "Curl initialization failed. Exiting...\n");
            curl_global_cleanup();
            abort();
        }
        /* get it! */
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        res = curl_easy_perform(curl_handle);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
      

        if (res != CURLE_OK)
        {
            
            cleanup(curl_handle, &recv_buf);
            decrease_active_threads();
            if(get_active_threads() == 0  && (get_local_total() >= n||queue_size() == 0))
            {
                sem_post(&complete);
                break;
            }
            sem_wait(&sem_queue);
            pthread_mutex_lock(&mutex);
            url = dequeue(q);
            pthread_mutex_unlock(&mutex);
     
            continue;
        }
        else
        {
            // printf("%lu bytes received in memory %p, seq=%d.\n",
            //        recv_buf.size, recv_buf.buf, recv_buf.seq);
        }

        /* process the download data */
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL); //can't be cancelled when processing data or we get a mutex issue
        process_data(curl_handle, &recv_buf);
        cleanup(curl_handle, &recv_buf);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        

        /* cleaning up */
        decrease_active_threads();
        if (get_active_threads() == 0 && (get_local_total() >= n||queue_size() == 0))
        {
            sem_post(&complete);
            break;
        }
        sem_wait(&sem_queue);
        pthread_mutex_lock(&mutex);
        url = dequeue(q);
        pthread_mutex_unlock(&mutex);

    }
    pthread_cleanup_pop(0);
    return 0;
}


int main(int argc, char **argv)
{
    xmlInitParser();
    double times[2];
    struct timeval tv;

    if (gettimeofday(&tv, NULL) != 0) {
        perror("gettimeofday");
        abort();
    }
    times[0] = (tv.tv_sec) + tv.tv_usec/1000000.;
    
    t = atoi(argv[2]);
	n = atoi(argv[4]);
    
    if (argc > 6){
        log_fname = argv[6];
        FILE *fp = fopen(log_fname, "a");
        if(n>0)
        {
            
            fprintf(fp, "%s\n", argv[7]);
            
        }
        fclose(fp);
        
        log_urls = 1;
    }
    FILE *fp2 = fopen("png_urls.txt", "a");
    fclose(fp2);
    char* url = argv[argc -1];
    
    hash_data = calloc(1,sizeof(struct hsearch_data));
    if(hcreate_r(50000, hash_data)== 0)
    {
        printf("CREATE FAILED\n");
    }
    q = create(1000);
    
    png_seq_ids = calloc(1, 60*sizeof(int));
    for(int i = 0; i< 60; i++)
    {
        png_seq_ids[i] = -3;
    }


    sem_init(&complete,0,0);
    sem_init(&sem_active_threads,0,1);
    sem_init(&sem_queue,0,0);
    sem_init(&sem_write_hash_table,0,1);

    void* vr;
    pthread_t *dobby_thread = malloc(sizeof(pthread_t));
    p_tids = malloc(sizeof(pthread_t) * t);
    for (int i = 0; i < t; i++)
    {
        pthread_create(p_tids + i, NULL, get_pngs, url); 
    }
    
    pthread_create(dobby_thread, NULL, &dobby, 0);
    pthread_join(*dobby_thread, &vr);

    if (gettimeofday(&tv, NULL) != 0) {
        perror("gettimeofday");
        abort();
    }
    times[1] = (tv.tv_sec) + tv.tv_usec/1000000.;
    printf("findpng2 execution time: %.6lf seconds\n",  times[1] - times[0]);

    free(p_tids);
    free(vr);
    free(dobby_thread);
    //DEBUG_VALIDATE_HASH_TABLE();
    hdestroy_r(hash_data);
    free(hash_data);
    free(png_seq_ids);
    free_queue(q);
    xmlCleanupParser();
    return 0;
}