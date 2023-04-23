#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include "eventbuf.h"


// FIFO queue for storing events
struct eventbuf *eb;

sem_t *spaces;
sem_t *items;
sem_t *mutex;

sem_t *sem_open_temp(const char *name, int value) {
    sem_t *sem;
    
    // Create the semaphore
    if ((sem = sem_open(name, O_CREAT, 0600, value)) == SEM_FAILED) {
        return SEM_FAILED;
    }

    // Unlink it so it will go away after this process exits
    if (sem_unlink(name) == -1) {
        sem_close(sem);
        return SEM_FAILED;
    }

    return sem;
}

struct params {
    int thread_num;
    int events;
};
    
void *prod_routine(void *arg);
void *con_routine(void *arg);

int main(int argc, char *argv[]) {

    // command line args should be 4
    if (argc != 5) {
        fprintf(stderr, "Error: Incorrect number of arguments, should be 4\n");
    }

    eb = eventbuf_create();

    // Convert all command line args from string to integer
    int num_prods = atoi(argv[1]);
    int num_cons = atoi(argv[2]);
    int num_events = atoi(argv[3]);
    int num_outstanding = atoi(argv[4]);

    // items semaphore, producers signal the consumers to wake up when data is ready to consume
    items = sem_open_temp("items", 0);
    // spaces semaphore, consumers signal the producers that there is room to add more items
    spaces = sem_open_temp("spaces", num_outstanding);
    // mutex semaphore (binary)
    mutex = sem_open_temp("mutex", 1);

    // arrays for threads
    pthread_t prod_th[num_prods];
    pthread_t con_th[num_cons];

    // creates producer threads number of times based off command line arg
    // arg passed to each start routine is thread id number
    for (int i = 0; i < num_prods; i++) {
        struct params *p = malloc(sizeof(struct params));
        p->thread_num = i;
        p->events = num_events;
        if (pthread_create(prod_th + i, NULL, prod_routine, (void*)p) != 0) {
            perror("Failed to create thread");
            return 1;
        } 
    }

    // creates consumer threads number of times based off command line arg
    // arg passed to each start routine is thread id number
    for (int i = 0; i < num_cons; i++) {
        struct params *c = malloc(sizeof(struct params));
        c->thread_num = i;
        c->events = num_events;
        if (pthread_create(con_th + i, NULL, con_routine, (void*)c) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    // joins producer threads number of times based off command line arg
    // arg passed to each start routine is thread id number
    for (int i = 0; i < num_prods; i++) {
        if (pthread_join(prod_th[i], NULL) != 0) {
            perror("Failed to join thread");
            return 1;
        } 
    }

    // post to items semaphore that all consumers are waiting on
    // fools consumer threads into thinking there are events to consume
    // consumer threads wake up to try and consume, find eventbuf empty, so they exit
    for (int i = 0; i < num_cons; i++) {
        sem_post(items);
    }

    // joins consumer threads number of times based off command line arg
    // arg passed to each start routine is thread id number
    for (int i = 0; i < num_cons; i++) {
        if (pthread_join(con_th[i], NULL) != 0) {
            perror("Failed to join thread");
            return 1;
        } 
    }
}

void *prod_routine(void *arg) {
    struct params *tdata = (struct params*)arg;
    int id = tdata->thread_num;
    int num_events = tdata->events;
    for (int i = 0; i < num_events; i++) {
        int event_id = id * 100 + i;
        sem_wait(spaces);
        sem_wait(mutex);
        printf("P%d: adding event %d\n", id, event_id);
        eventbuf_add(eb, event_id);
        sem_post(mutex);
        sem_post(items);
    }
    printf("P%d: exiting\n", id);
    free(tdata);
    return NULL;
}

void *con_routine(void *arg) {
    struct params *tdata = (struct params*)arg;
    int id = tdata->thread_num;
    int num_events = tdata->events;
    for (int i = 0; i < num_events; i++) {
        sem_wait(items);
        sem_wait(mutex);
        if (eventbuf_empty(eb)) {
            sem_post(mutex);
            break;
        }
        int event_id = eventbuf_get(eb);
        printf("C%d: got event %d\n", id, event_id);
        sem_post(mutex);
        sem_post(spaces);
    }
    printf("C%d: exiting\n", id);
    free(tdata);
    return NULL;
}