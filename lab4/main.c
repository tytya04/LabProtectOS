#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

mode_t parse_numeric_mode(const char *mode_str) {
    mode_t mode = 0;

    if (strlen(mode_str) != 3) {
        fprintf(stderr, "Invalid numeric mode format\n");
        exit(EXIT_FAILURE);
    }

    mode = ((mode_str[0] - '0') * 64) + ((mode_str[1] - '0') * 8) + (mode_str[2] - '0');
    return mode;
}

void apply_symbolic_mode(const char *mode_str, mode_t *current_mode) {
    char who[10];
    mode_t add_mask = 0, remove_mask = 0;

    int i = 0;
    while (mode_str[i]) {
        int who_index = 0;
        while (mode_str[i] == 'u' || mode_str[i] == 'g' || mode_str[i] == 'o' || mode_str[i] == 'a') {
            who[who_index++] = mode_str[i];
            i++;
        }

        if (strlen(who) == 0) {
            who[who_index++] = 'a';
        }

        who[who_index] = '\0';

        if (mode_str[i] == '+' || mode_str[i] == '-') {
            char op = mode_str[i];
            i++;

            // Разбор прав доступа
            while (mode_str[i] && (mode_str[i] == 'r' || mode_str[i] == 'w' || mode_str[i] == 'x')) {
                switch (mode_str[i]) {
                    case 'r':
                        for (int j = 0; j < who_index; j++) {
                            if (who[j] == 'u') add_mask |= S_IRUSR;
                            if (who[j] == 'g') add_mask |= S_IRGRP;
                            if (who[j] == 'o') add_mask |= S_IROTH;
                            if (who[j] == 'a') {
                                add_mask |= S_IRUSR | S_IRGRP | S_IROTH;
                            }
                        }
                        if (op == '-') remove_mask |= add_mask;
                        break;
                    case 'w':
                        for (int j = 0; j < who_index; j++) {
                            if (who[j] == 'u') add_mask |= S_IWUSR;
                            if (who[j] == 'g') add_mask |= S_IWGRP;
                            if (who[j] == 'o') add_mask |= S_IWOTH;
                            if (who[j] == 'a') {
                                add_mask |= S_IWUSR | S_IWGRP | S_IWOTH;
                            }
                        }
                        if (op == '-') remove_mask |= add_mask;
                        break;
                    case 'x':
                        for (int j = 0; j < who_index; j++) {
                            if (who[j] == 'u') add_mask |= S_IXUSR;
                            if (who[j] == 'g') add_mask |= S_IXGRP;
                            if (who[j] == 'o') add_mask |= S_IXOTH;
                            if (who[j] == 'a') { // Если 'a', добавляем для всех
                                add_mask |= S_IXUSR | S_IXGRP | S_IXOTH;
                            }
                        }
                        if (op == '-') remove_mask |= add_mask;
                        break;
                }
                i++;
            }

            if (op == '+') {
                *current_mode |= add_mask;
            } else if (op == '-') {
                *current_mode &= ~remove_mask;
            }

            add_mask = 0;
            remove_mask = 0;
        } else {
            i++;
        }
    }
}

void print_permissions(mode_t mode) {
    printf("User: ");
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");

    printf(" Group: ");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");

    printf(" Others: ");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");

    printf("\n");
}

void mode_to_string(mode_t mode, char *buffer) {
    for (int i = 0; i < 9; i++) {
        if (mode & (1 << (8 - i))) {
            buffer[i] = (i % 3 == 0) ? 'r' : (i % 3 == 1) ? 'w' : 'x';
        } else {
            buffer[i] = '-';
        }
    }
    buffer[9] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <mode> <file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *mode_str = argv[1];
    const char *file_name = argv[2];

    struct stat file_stat;

    if (stat(file_name, &file_stat) < 0) {
        perror("stat");
        return EXIT_FAILURE;
    }

    mode_t old_mode = file_stat.st_mode;
    mode_t new_mode = old_mode;

    if (mode_str[0] >= '0' && mode_str[0] <= '9') {
        new_mode = parse_numeric_mode(mode_str);
    } else {
        apply_symbolic_mode(mode_str, &new_mode);
    }

    if (chmod(file_name, new_mode) < 0) {
        perror("chmod");
        return EXIT_FAILURE;
    }

    // Преобразуем старые и новые права в строковый формат
    char oldPerms[10], newPerms[10];
    mode_to_string(old_mode, oldPerms);
    mode_to_string(new_mode, newPerms);

    printf("Права доступа для '%s' изменены с %s на %s\n", file_name, oldPerms, newPerms);

    return EXIT_SUCCESS;
}
