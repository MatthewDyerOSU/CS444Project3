#include <stdlib.h>
#include <pthreads.h>
#include "eventbuf.h"

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
    struct eventbuf;
}
    
void *prod_routine(void *arg) {
    int id = (struct params*)arg->thread_num;
    eventbuf_add(((struct params*)arg->eventbuf), 1);
}

void *con_routine(void *arg) {

}

int main(int argc, char *argv[]) {

    // command line args should be 4
    if (argc != 4) {
        fprintf(stderr, "Error: Incorrect number of arguments, should be 4\n");
    }

    // Convert all command line args from string to integer
    int num_prods = atoi(argv[0]);
    int num_cons = atoi(argv[1]);
    int num_events = atoi(argv[2]);
    int num_outstanding = atoi(argv[3]);

    // Initialize the FIFO queue for storing events
    struct eventbuf *eb;
    eb = eventbuf_create();

    // Initialize structs to hold params to be passed to thread start routines
    struct params *prod_params = (struct params *)malloc(sizeof(struct params))
    struct params *con_params = (struct params *)malloc(sizeof(struct params))

    // set both struct eventbufs to same eventbuf
    prod_params->eventbuf = eb;
    con_params->eventbuf = eb;

    // one sem for producers and one for consumers
    sem_open_temp("prod_sem", num_outstanding);
    sem_open_temp("con_sem", num_outstanding);

    // arrays for threads
    pthread_t prod_th[num_prods];
    pthread_t con_th[num_cons];

    // creates and joins producer threads number of times based off command line arg
    // arg passed to each start routine is thread id number
    for (int i = 0; i < num_prods-1; i++) {
        prod_params->thread_num = i;
        if (pthread_create(prod_th + i, NULL, &prod_routine, prod_params) != 0) {
            perror("Failed to create thread");
            return 1;
        }
        
        if (pthread_join(prod_th[i], NULL) != 0) {
            perror("Failed to join thread");
            return 1;
        } 
    }

    // creates and joins consumer threads number of times based off command line arg
    // arg passed to each start routine is thread id number
    for (int i = 0; i < num_cons-1; i++) {
        con_params->thread_num = i;
        if (pthread_create(con_th + i, NULL, &con_routine, con_params) != 0) {
            perror("Failed to create thread");
            return 1;
        }
        
        if (pthread_join(con_th[i], NULL) != 0) {
            perror("Failed to join thread");
            return 1;
        } 
    }


}
  
 
 

