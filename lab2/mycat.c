#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

void print_file(FILE *file, bool number_all, bool number_nonempty, bool show_ends) {
    char line[1024];
    int line_number = 1; // Номер текущей строки
    bool print_line_number; // Флаг для печати номера строки

    while (fgets(line, sizeof(line), file) != NULL) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0'; // Убираем символ новой строки
        }

        print_line_number = false; // Сбрасываем флаг перед каждой строкой

        // Определяем, нужно ли печатать номер строки
        if (number_all) {
            print_line_number = true;
        } else if (number_nonempty && len > 1) { // Печатаем номер только для непустых строк
            print_line_number = true;
        }

        if (print_line_number) {
            printf("%6d  ", line_number++);
        }

        printf("%s", line);

        if (show_ends) {
            printf("$"); // Отображаем символ конца строки
        }

        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    bool number_all = false;
    bool number_nonempty = false;
    bool show_ends = false;
    int opt;

    while ((opt = getopt(argc, argv, "nbE")) != -1) {
        switch (opt) {
            case 'n':
                number_all = true;
                break;
            case 'b':
                number_nonempty = true;
                number_all = false; // Отключаем number_all, если задан флаг -b
                break;
            case 'E':
                show_ends = true;
                break;
            default:
                fprintf(stderr, "Usage: %s [-n] [-b] [-E] [file ...]\n", argv[0]);
                return 1;
        }
    }

    if (optind >= argc) {
        print_file(stdin, number_all, number_nonempty, show_ends);
    } else {
        // Проходим по каждому указанному файлу
        for (int i = optind; i < argc; i++) {
            FILE *file = fopen(argv[i], "r");
            if (!file) {
                perror("fopen");
                continue; 
            }
            print_file(file, number_all, number_nonempty, show_ends);
            fclose(file);
        }
    }




    return 0;
}
