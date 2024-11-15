#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

typedef struct {
    pid_t pid;       // процесс, отправивший сообщение
    char text[12];   
    time_t time;     
} message;

/* ~ Дескриптор 0 - на чтение
   ~ Дескриптор 1 - на запись */

int main() {
    int pid; // место для хранения идентификатора дочернего процесса
    char *fifo_name = "fifoFile"; 

    unlink("fifoFile"); // удаляем ранее существующий FIFO-файл, если он есть

    // именованный канал (FIFO) с правами доступа 0666 (ч/з для всех)
    if (mkfifo(fifo_name, 0666) == -1) {
        printf("mkfifo() failed\n");
        return 1;
    }

    pid = fork();// создаем дочерний процесс

    if (pid == -1) {
        printf("fork() failed\n");
        return 1;
    }

    if (pid == 0) { //в дочернем процессе
        int read_fd = open("fifoFile", O_RDONLY);

        if (read_fd == -1) {
            printf("open write_fd error\n");
            return 1;
        }

        message* cMessage = malloc(sizeof(message));
        read(read_fd, cMessage, sizeof(message));

        time_t currentTime = 0;
        time(&currentTime);

        printf("Получаемое сообщение (дочерний процесс): %d %s %s", cMessage->pid, cMessage->text, ctime(&cMessage->time));
        printf("Текущее время: %s", ctime(&currentTime));

        close(read_fd);
        free(cMessage);

        return 0;
    } else { // в родительском процессе
        int write_fd = open("fifoFile", O_WRONLY);

        if (write_fd == -1) {
            printf("open write_fd error\n");
            return 1;
        }

        message* pMessage = malloc(sizeof(message));

        pMessage->pid = getpid(); 
        strcpy(pMessage->text, "Hello World"); 
        time(&pMessage->time); 

        printf("Sent message: %d %s %s", pMessage->pid, pMessage->text, ctime(&pMessage->time));
        sleep(5);

        if (write(write_fd, pMessage, sizeof(message)) == -1) {
            printf("Write error\n"); 
            return 1;
        }

        free(pMessage);
        close(write_fd);
    }
    return 0;
}
