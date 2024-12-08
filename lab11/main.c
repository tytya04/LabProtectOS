#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_READERS 10
#define BUFFER_SIZE 100

char buffer[BUFFER_SIZE];
pthread_rwlock_t rwlock;
int recordCounter = 0;

void* writerThread(void* arg) {
    while (1) {
        pthread_rwlock_wrlock(&rwlock);

        sprintf(buffer, "Record #%d", recordCounter);
        recordCounter++;
        sleep(1);
        pthread_rwlock_unlock(&rwlock);

        usleep(10);
    }
    return NULL;
}

void* readerThread(void* arg) {
    long tid = (long)arg;
    while (1) {
        pthread_rwlock_rdlock(&rwlock);
		printf("Reader %ld tid %lx: %s\n", tid, pthread_self(),buffer);
        pthread_rwlock_unlock(&rwlock);
        
        usleep(10);
    }
    return NULL;
}

int main() {
    pthread_t writer;
    pthread_t readers[NUM_READERS];
    
    pthread_rwlock_init(&rwlock, NULL);
    
    if (pthread_create(&writer, NULL, writerThread, NULL) != 0) {
        fprintf(stderr, "Ошибка при создании пишущего потока\n");
        return 0;
    }
    
    for (long i = 0; i < NUM_READERS; i++) {
    	if (pthread_create(&readers[i], NULL, readerThread, (void*)i) != 0) {
            fprintf(stderr, "Ошибка при создании читающего потока %ld\n", i);
            return 0;
        }
    }
    
    if (pthread_join(writer, NULL) != 0) {
        fprintf(stderr, "Ошибка при ожидании завершения пишущего потока\n");
        return 0;
    }
    
    for (long i = 0; i < NUM_READERS; i++) {
        if (pthread_join(readers[i], NULL) != 0) {
            fprintf(stderr, "Ошибка при ожидании завершения читающего потока %ld\n", i);
            return 0;
        }
    }
    
    pthread_rwlock_destroy(&rwlock);
    
    return 0;
}