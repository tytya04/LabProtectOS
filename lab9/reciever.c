#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <time.h>

#define SHARED_MEMORY_KEY 13
#define MAX_BUFFER_SIZE 256
const size_t FLAG_FILE = 0666;

typedef struct {
    time_t timestamp;
    pid_t pid;
    char message[MAX_BUFFER_SIZE];
} DataPacket;

struct sembuf sem_lock = {0, -1, 0};
struct sembuf sem_open = {0, 1, 0};

int main() {
    int shmId = shmget((key_t)SHARED_MEMORY_KEY, sizeof(DataPacket), FLAG_FILE);
    if (shmId == -1) {
        perror("Ошибка при получении идентификатора разделяемой памяти");
        return 0;
    }

    DataPacket *sharedData = (DataPacket *)shmat(shmId, NULL, 0);
    if (sharedData == (void *)-1) {
        perror("Ошибка при присоединении разделяемой памяти");
        return 0;
    }
	
	int sem = semget((key_t)SHARED_MEMORY_KEY, 1, IPC_CREAT | FLAG_FILE);
	if (sem == -1) {
		perror("Ошибка при создании семафора");
        return 0;
    }
	
	semop(sem, &sem_lock, 1);
    sleep(1);
    time_t now;
    time(&now);
    printf("Получатель:\n");
    printf("Time: %s", ctime(&sharedData->timestamp));
    printf("PID отправителя: %d\n", sharedData->pid);
    printf("Message: %s\n", sharedData->message);
    printf("Текущее время: %s", ctime(&now));
    printf("-----------------------------\n");
	semop(sem, &sem_open, 1);
    
    if (shmdt(sharedData) == -1) {
        perror("Ошибка при отсоединении разделяемой памяти");
        return 0;
    }

    return 0;
}
