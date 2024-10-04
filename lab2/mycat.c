#include <stdio.h>      
#include <stdlib.h>     
#include <stdbool.h>    
#include <string.h>     
#include <unistd.h>     

void print_file(FILE *file, bool number_all, bool number_nonempty, bool show_ends) {
    char line[1024];       
    int line_number = 1;   
    bool print_line_number; 

    while (fgets(line, sizeof(line), file) != NULL) {
        // Убираем символ новой строки, если он есть
        size_t len = strlen(line); 
        if (line[len - 1] == '\n') { 
            line[len - 1] = '\0'; // Заменяем символ новой строки на символ конца строки
        }

        print_line_number = false; // Сброс флага перед каждой строкой

        // Определяем, нужно ли печатать номер строки
        if (number_all) { 
            print_line_number = true;
        } else if (number_nonempty && len > 1) { // флаг -b и строка не пустая
            print_line_number = true;
        }

        // номер строки
        if (print_line_number) {
            printf("%6d  ", line_number++); 
        }

        printf("%s", line);

        // флаг -E
        if (show_ends) {
            printf("$");
        }

        printf("\n"); 
    }
}

int main(int argc, char *argv[]) {
    bool number_all = false;      // нумерации всех строк
    bool number_nonempty = false; // нумерации ненулевых строк
    bool show_ends = false;       // отображение конца строки
    int opt;                      

    while ((opt = getopt(argc, argv, "nbE")) != -1) {
        switch (opt) {
            case 'n': 
                number_all = true;
                break;
            case 'b': 
                number_nonempty = true;
                break;
            case 'E': 
                show_ends = true;
                break;
            default:
                fprintf(stderr, "Usage: %s [-n] [-b] [-E] [file ...]\n", argv[0]);
                return 1; // Возвращаем код ошибки
        }
    }

    if (optind >= argc) {
        print_file(stdin, number_all, number_nonempty, show_ends);
    } else {
        for (int i = optind; i < argc; i++) {
            FILE *file = fopen(argv[i], "r"); 
            if (!file) { 
                perror("fopen"); 
                return 1; 
            }
            print_file(file, number_all, number_nonempty, show_ends);
            fclose(file); 
        }
    }

    return 0;
}
