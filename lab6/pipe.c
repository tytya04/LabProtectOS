#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

typedef struct {
    pid_t pid;       // процесс, отправивший сообщение
    char text[12];   
    time_t time;    
} message;

int main() {
    int pipe_fd[2]; // место для хранения файловых дескрипторов канала (pipe)
    int pid;        // переменная для хранения идентификатора дочернего процесса

    // создаем канал, который будет использоваться для межпроцессного взаимодействия
    if (pipe(pipe_fd)) {
        printf("Pipe error\n"); 
        return 1;
    }

    pid = fork(); // очерний процесс

    if (pid == -1) {
        printf("fork() failed\n");

        close(pipe_fd[0]);
        close(pipe_fd[1]);

        return 1;
    }

    if (pid == 0) { // этот блок выполняется в дочернем процессе
        // выделяем память для структуры сообщения, которое будет получено
        message* cMessage = malloc(sizeof(message));

        close(pipe_fd[1]); // закрываем конец записи канала, так как дочерний процесс только читает

        read(pipe_fd[0], cMessage, sizeof(message)); // Читаем данные из канала в структуру сообщения

        time_t currentTime = 0;
        time(&currentTime);

        printf("Received message: %d %s %s", cMessage->pid, cMessage->text, ctime(&cMessage->time));
        printf("Current time: %s", ctime(&currentTime));

        close(pipe_fd[0]);
        free(cMessage);

        return 0;
    } else { // блок в родительском процессе
        // Выделяем память для структуры сообщения, которое будет отправлено
        message* pMessage = malloc(sizeof(message));

        pMessage->pid = getpid();
        strcpy(pMessage->text, "Hello World");
        time(&pMessage->time);

        printf("Sent message: %d %s %s", pMessage->pid, pMessage->text, ctime(&pMessage->time));

        sleep(5);
        close(pipe_fd[0]);

        if (write(pipe_fd[1], pMessage, sizeof(message)) == -1) {
            printf("Write error\n"); 
            return 1;
        }

        free(pMessage);
        close(pipe_fd[1]);
    }

    return 0;
}
