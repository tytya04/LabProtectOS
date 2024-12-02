#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>

#define SHARED_MEMORY_KEY 24
#define MAX_BUFFER_SIZE 256
const size_t FLAG_FILE = 0666;

typedef struct {
    time_t timestamp;
    pid_t pid;
    char message[MAX_BUFFER_SIZE];
} DataPacket;

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

    while (1) {
        time_t now;
        time(&now);
        printf("Получатель:\n");
        printf("Time: %s", ctime(&sharedData->timestamp));
        printf("PID отправителя: %d\n", sharedData->pid);
        printf("Message: %s\n", sharedData->message);
        printf("Текущее время: %s", ctime(&now));
        printf("-----------------------------\n");

        sleep(1);
    }

    if (shmdt(sharedData) == -1) {
        perror("Ошибка при отсоединении разделяемой памяти");
        return 0;
    }

    return 0;
}
