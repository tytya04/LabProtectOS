#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     

void print_highlighted(const char *line, const char *pattern) {
    const char *ptr = line; // текущая позиция
    const char *match;      // для хранения совпадений

    // пока есть совпадения
    while ((match = strstr(ptr, pattern)) != NULL) {
        // часть строки до совпадения
        fwrite(ptr, 1, match - ptr, stdout);
        printf("\033[1;31m%.*s\033[0m", (int)strlen(pattern), match);
        // указатель в конец совпадения, чтобы продолжить поиск 
        ptr = match + strlen(pattern);
    }
    // оставшуюся часть строки после совпадения
    printf("%s", ptr);
}

void grep_file(FILE *file, const char *pattern) {
    char line[1024];

    while (fgets(line, sizeof(line), file) != NULL) {
        if (strstr(line, pattern) != NULL) { // содержит ли строка то что ищем
            print_highlighted(line, pattern);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s pattern [file]\n", argv[0]);
        return 1;
    }

    const char *pattern = argv[1];

    if (argc == 2) {
        grep_file(stdin, pattern); 
    } else {
        FILE *file = fopen(argv[2], "r"); 
        if (!file) { 
            perror("fopen"); 
            return 1; 
        }
        grep_file(file, pattern);
        fclose(file); 
    }

    return 0; 
}
