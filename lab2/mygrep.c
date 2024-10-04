#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     

void print_highlighted(const char *line, const char *pattern) {
    const char *ptr = line; // текущая позицию в строке
    const char *match;      // для хранения адреса найденного совпадения

    // пока есть совпадения в строке
    while ((match = strstr(ptr, pattern)) != NULL) {
        // Печатаем часть строки до найденного совпадения
        fwrite(ptr, 1, match - ptr, stdout);
        // Печатаем найденное совпадение с красной подсветкой
        printf("\033[1;31m%.*s\033[0m", (int)strlen(pattern), match);
        // Передвигаем указатель на конец совпадения, чтобы продолжить поиск в остальной части строки
        ptr = match + strlen(pattern);
    }
    // Печатаем остаток строки после последнего найденного совпадения
    printf("%s", ptr);
}

void grep_file(FILE *file, const char *pattern) {

    while (fgets(line, sizeof(line), file) != NULL) {
        if (strstr(line, pattern) != NULL) {// Проверяем, содержит ли строка искомый шаблон
            print_highlighted(line, pattern);
        }
    }
}

// Главная функция программы
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s pattern [file]\n", argv[0]);
        return 1;
    }

    // Сохраняем шаблон для поиска
    const char *pattern = argv[1];

    // Если передан только шаблон, читаем из стандартного ввода
    if (argc == 2) {
        grep_file(stdin, pattern); // Вызываем функцию для чтения из stdin
    } else {
        // Если указан файл, открываем его для чтения
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
