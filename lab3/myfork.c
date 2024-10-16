#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

//(Ctrl+C)
void handle_sigint(int sig) {
    printf("Получен SIGINT (Ctrl+C), номер сигнала: %d\n", sig);
    exit(1);
}

void handle_sigterm(int sig) { printf("Получен SIGTERM, номер сигнала: %d\n", sig); }

void on_exit_function() { printf("Программа завершает работу...\n"); }

int main() {
    pid_t pid;
    struct sigaction sa;

    signal(SIGINT, handle_sigint);

    // Настройка обработчика для SIGTERM
    sa.sa_handler = handle_sigterm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);

    
    atexit(on_exit_function);// Регистрация функции, при завершении программы

    pid = fork();

    if (pid < 0) {
        perror("Ошибка: не удалось создать новый процесс");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        printf("Дочерний процесс: PID = %d, PPID = %d\n", getpid(), getppid());
        sleep(10);  // Задержка на 10 секунд
        exit(42);   // Завершение дочернего процесса с кодом 42
    } else {
        printf("Родительский процесс: PID = %d, PID дочернего процесса = %d\n", getpid(), pid);

        int status;
        wait(&status);

        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            printf("Родительский процесс: дочерний процесс завершился с кодом %d\n", exit_status);
        } else {
            printf("Родительский процесс: дочерний процесс завершился с ошибкой\n");
        }
    }

    return 0;
}
