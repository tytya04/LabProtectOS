#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#define MAX_FILES 1024

extern char *optarg;
extern int optind, opterr, optopt;

typedef struct {
    char name[256];
    struct stat st;
    bool is_symlink;
} FileEntry;

int compare(const void *a, const void *b) {
    const char *nameA = ((FileEntry *)a)->name;
    const char *nameB = ((FileEntry *)b)->name;

    if (strcmp(nameA, ".") == 0) return -1;
    if (strcmp(nameB, ".") == 0) return 1;
    if (strcmp(nameA, "..") == 0) return -1;
    if (strcmp(nameB, "..") == 0) return 1;

    return strcmp(nameA, nameB);
}

void list_directory(const char *path, bool show_all, bool long_format) {
    DIR *dir;
    struct dirent *entry;
    FileEntry files[MAX_FILES];
    int count = 0;
    blkcnt_t total_blocks = 0;

    dir = opendir(path);
    if (!dir) {
        perror("diropen");
        return;
    }

    char full_path[1024];  

    int max_nlink_len = 0;
    int max_owner_len = 0;
    int max_group_len = 0;
    int max_size_len = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (!show_all && entry->d_name[0] == '.') continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat file_stat;
        if (lstat(full_path, &file_stat) == -1) {
            perror("stat");
            continue;
        }

        files[count].is_symlink = S_ISLNK(file_stat.st_mode);
        total_blocks += file_stat.st_blocks;
        strncpy(files[count].name, entry->d_name, sizeof(files[count].name));
        files[count].st = file_stat;
        
        // Определение максимальной длины для выравнивания столбцов
        max_nlink_len = (snprintf(NULL, 0, "%lu", (unsigned long)file_stat.st_nlink) > max_nlink_len) 
                         ? snprintf(NULL, 0, "%lu", (unsigned long)file_stat.st_nlink) : max_nlink_len;

        struct passwd *pw = getpwuid(file_stat.st_uid);
        struct group *gr = getgrgid(file_stat.st_gid);
        max_owner_len = (pw && strlen(pw->pw_name) > max_owner_len) ? strlen(pw->pw_name) : max_owner_len;
        max_group_len = (gr && strlen(gr->gr_name) > max_group_len) ? strlen(gr->gr_name) : max_group_len;

        max_size_len = (snprintf(NULL, 0, "%lld", (long long)file_stat.st_size) > max_size_len) 
                        ? snprintf(NULL, 0, "%lld", (long long)file_stat.st_size) : max_size_len;
        
        count++;
    }

    closedir(dir);

    qsort(files, count, sizeof(FileEntry), compare);

    if (long_format) {
        printf("total %lld\n", (long long)(total_blocks / 2)); // Вывод общего количества блоков
    }

    for (int i = 0; i < count; i++) {
        if (long_format) {
            uid_t uid = files[i].st.st_uid;
            gid_t gid = files[i].st.st_gid;
            struct passwd *pw = getpwuid(uid);
            struct group *gr = getgrgid(gid);
            char time[40];
            strftime(time, 40, "%b %d %H:%M", localtime(&(files[i].st.st_mtime)));

            const char* owner_name = (pw != NULL) ? pw->pw_name : "";
            const char* group_name = (gr != NULL) ? pw->pw_name : "";

            char uid_str[32], gid_str[32];
            if (pw == NULL) {
                snprintf(uid_str, sizeof(uid_str), "%u", uid);
                owner_name = uid_str;
            }
            if (gr == NULL) {
                snprintf(gid_str, sizeof(gid_str), "%u", gid);
                group_name = gid_str;
            }

            // Выводим информацию о файле
            printf("%s%s%s%s%s%s%s%s%s%s %*lu %-*s %-*s %*lld %s ",
                (S_ISDIR(files[i].st.st_mode)) ? "d" : (files[i].is_symlink ? "l" : "-"),
                (files[i].st.st_mode & S_IRUSR) ? "r" : "-",
                (files[i].st.st_mode & S_IWUSR) ? "w" : "-",
                (files[i].st.st_mode & S_IXUSR) ? "x" : "-",
                (files[i].st.st_mode & S_IRGRP) ? "r" : "-",
                (files[i].st.st_mode & S_IWGRP) ? "w" : "-",
                (files[i].st.st_mode & S_IXGRP) ? "x" : "-",
                (files[i].st.st_mode & S_IROTH) ? "r" : "-",
                (files[i].st.st_mode & S_IWOTH) ? "w" : "-",
                (files[i].st.st_mode & S_IXOTH) ? "x" : "-",
                max_nlink_len, (unsigned long)files[i].st.st_nlink,  // Ссылки
                max_owner_len, owner_name,  // Имя владельца
                max_group_len, group_name,  // Имя группы
                max_size_len, (long long)files[i].st.st_size,  // Размер
                time  // Время
            );

            // Меняем цвет только для названия файла
            if (S_ISDIR(files[i].st.st_mode)) {
                printf("\033[1;34m%s\033[0m\n", files[i].name);  // Синий для директорий
            } else if (S_ISREG(files[i].st.st_mode) && (files[i].st.st_mode & S_IXUSR)) {
                printf("\033[1;32m%s\033[0m\n", files[i].name);  // Зеленый для исполняемых файлов
            } else if (files[i].is_symlink) {
                printf("\033[1;36m%s\033[0m", files[i].name);  // Бирюзовый для символических ссылок
                char target[1024];
                ssize_t len = readlink(full_path, target, sizeof(target) - 1);
                if (len != -1) {
                    target[len] = '\0';
                    printf(" -> %s\n", target);
                } else {
                    perror("readlink");
                }

            } else {
                printf("%s\n", files[i].name);  // Обычный файл без выделения
            }

    

        } else {
            // Изменяем цвета для короткого формата
            if (S_ISDIR(files[i].st.st_mode)) {
                printf("\033[1;34m%s\033[0m  ", files[i].name);  // Синий для директорий
            } else if (S_ISREG(files[i].st.st_mode) && (files[i].st.st_mode & S_IXUSR)) {
                printf("\033[1;32m%s\033[0m  ", files[i].name);  // Зеленый для исполняемых файлов
            } else if (files[i].is_symlink) {
                printf("\033[1;36m%s\033[0m  ", files[i].name);  // Бирюзовый для символических ссылок
            } else {
                printf("%s  ", files[i].name);  // Обычный файл без выделения
            }
        }
    }
    printf("\n");  // Переход на новую строку после вывода всех файлов
}

int main(int argc, char *argv[]) {
    const char *path = ".";
    bool show_all = false;
    bool long_format = false;

    // Обрабатываем флаги
    int opt;
    while ((opt = getopt(argc, argv, "al")) != -1) {
        switch (opt) {
            case 'a':
                show_all = true;
                break;
            case 'l':
                long_format = true;
                break;
            default:
                fprintf(stderr, "Usage: %s [-a] [-l] [path]\n", argv[0]);
                return 1;
        }
    }

    // Проверяем, указан ли путь
    if (optind < argc) {
        path = argv[optind];  // Если указан путь, используем его
    }

    // Вызываем функцию для вывода содержимого директории
    list_directory(path, show_all, long_format);

    return 0;
}