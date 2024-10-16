#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iso646.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>

// изменение режима доступа файла
mode_t makeNewMode(char* fileName, char* ugoa, char action, char* change) {
    struct stat st;

    if(lstat(fileName, &st) != 0) {
        printf("chmod: невозможно получить доступ к '%s': %s\n", fileName, strerror(2));
        exit(EXIT_FAILURE);
    }

    bool userMode = false, groupMode = false, otherMode = false;

    // Установка режимов доступа на основе входных данных
    if(strlen(ugoa) == 0) {
        userMode = groupMode = otherMode = true;  // Все пользователи
    } else {
        for(int i = 0; i < strlen(ugoa); i++) {
            if(ugoa[i] == 'u') { userMode = true; }
            else if(ugoa[i] == 'g') { groupMode = true; }
            else if(ugoa[i] == 'o') { otherMode = true; }
            else if(ugoa[i] == 'a') { userMode = groupMode = otherMode = true; } // Все
            else {
                fprintf(stderr, "Некорректное значение! Поддерживаемые: 'ugoa'\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    mode_t newMode = st.st_mode;  // создание нового режима доступа
    for(int i = 0; i < strlen(change); i++) {
        mode_t bitMask = 0;

        // битовая маска для прав доступа
        if(change[i] == 'r') {
            if(userMode) { bitMask |= S_IRUSR; }
            if(groupMode) { bitMask |= S_IRGRP; }
            if(otherMode) { bitMask |= S_IROTH; }
        } else if(change[i] == 'w') {
            if(userMode) { bitMask |= S_IWUSR; }
            if(groupMode) { bitMask |= S_IWGRP; }
            if(otherMode) { bitMask |= S_IWOTH; }
        } else if(change[i] == 'x') {
            if(userMode) { bitMask |= S_IXUSR; }
            if(groupMode) { bitMask |= S_IXGRP; }
            if(otherMode) { bitMask |= S_IXOTH; }
        } else {
            fprintf(stderr, "Некорректное значение! Поддерживаемые: 'rwx'\n");
            exit(EXIT_FAILURE);
        }

        // Изменение режима доступа на основе действия ('+', '-', '=')
        switch ((int)action) {
            case 43: // '+' 
                newMode |= bitMask;  // Добавить права
                break;
            case 45: // '-'
                newMode &= ~bitMask;  // Удалить права
                break;
            case 61: // '='
                newMode = (newMode & ~bitMask) | bitMask; // Установить новые права
                break;
            default:
                fprintf(stderr, "Некорректная операция! Поддерживаемые: '+', '-', '='\n");
                exit(EXIT_FAILURE);
        }
    }
    return newMode; 
}

// преобразование числового режима в формат mode_t
mode_t numericalMode(char* mode) {
    unsigned long int newMode = atoi(mode);
    return ((newMode / 100) << 6) | ((newMode / 10 % 10) << 3) | (newMode % 10);
}

int main(int argc, char** argv) {
    if(argc != 3) {
        puts("Недостаточно входных данных");
        exit(EXIT_FAILURE);
    }

    char *command = argv[1];  // Команда для изменения прав
    char *fileName = argv[2]; 

    // выделение памяти для режимов доступа
    char *ugoa = calloc(100, sizeof(char));
    char *change = calloc(10, sizeof(char));
    char action;
    bool actionFound = false;
    int count = 0;

    // Разделение команды на действия и изменения
    for(int i = 0; i < strlen(command); i++) {
        if(command[i] == '+' or command[i] == '-' or command[i] == '=') {
            action = command[i];
            actionFound = true;  // Действие найдено
        } else {
            if(actionFound == true) {
                change[count] = command[i];
                count++;
            } else {
                ugoa[i] = command[i];  // Установка режима
            }
        }
    }

    mode_t numMode = 0;  // Переменная для числового режима
    if(strlen(change) == 0) { // Если изменений нет, то используем числовой режим
        numMode = numericalMode(ugoa);
        if(chmod(fileName, numMode) == -1) {
            perror("Ошибка при определении новых прав доступа");
            exit(EXIT_FAILURE);
        }
    } else {
        mode_t mode = makeNewMode(fileName, ugoa, action, change);
        if(chmod(fileName, mode) == -1) {
            perror("Ошибка при определении новых прав доступа");
            exit(EXIT_FAILURE);
        }
    }

    free(ugoa);
    free(change);
    return 0;
}
