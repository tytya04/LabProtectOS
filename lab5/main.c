#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

typedef struct {
    struct stat fileStat; // Инфа о файле
    char fileName[20];    // Имя файла
    time_t time;          // Время добавления файла в архив
} fileInfo;

int createArchive(char *filename) {
    int archive = open(filename, O_WRONLY); 
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // Устанавливаем права на чтение/запись для владельца и группы

    if (archive == -1) { archive = creat(filename, mode); } // Если архив не существует, создаем его
    close(archive);
    return (archive == -1) ? 1 : 0; 
}

int readArchive(char *archname) {
    int archive = open(archname, O_RDONLY); 
    if (archive == -1) return 1;

    size_t count = 0;
    while (1) {
        fileInfo a;
        if (!read(archive, &a, sizeof(fileInfo))) break; 

        printf("Имя файла: %s\n", a.fileName);
        printf("Вес файла: %ld\n", a.fileStat.st_size);
        printf("Время добавления в архив: %s\n", ctime(&a.time));
        lseek(archive, a.fileStat.st_size, SEEK_CUR); // Пропускаем содержимое файла
        count++;
    }
    printf("Всего файлов в архиве: %ld\n", count);
    close(archive);
    return 0;
}

int addFile(char *archname, char *filename) {
    if (strcmp(archname, filename) == 0) { 
        printf("Архив не может архивировать сам себя\n");
        return 1;
    }

    int archive = open(archname, O_RDONLY); 
    if (archive == -1) {
        if (createArchive(archname) != 0) {
            printf("Ошибка создания архива %s\n", archname);
            return 1;
        }
        archive = open(archname, O_RDONLY);
    }

    int file = open(filename, O_RDONLY); 
    if (file == -1) return 1;

    bool fileExist = false;
    fileInfo a;

    // есть ли уже файл в архиве
    while (read(archive, &a, sizeof(fileInfo)) == sizeof(fileInfo)) {
        if (strcmp(a.fileName, filename) == 0) {
            fileExist = true;
            break;
        }
        lseek(archive, a.fileStat.st_size, SEEK_CUR); // Пропускаем содержимое файла
    }
    close(archive);

    if (fileExist) {
        printf("Файл %s уже находится в архиве\n", filename);
        close(file);
        return 1;
    }

    // Если файл еще не существует в архиве, добавляем его
    archive = open(archname, O_WRONLY | O_APPEND);
    if (archive == -1) return 1;

    struct stat file_info;
    if (stat(filename, &file_info) != 0) return 1;

    time_t now;
    time(&now);

    fileInfo newFileInfo;
    strncpy(newFileInfo.fileName, filename, sizeof(newFileInfo.fileName) - 1);
    newFileInfo.fileName[sizeof(newFileInfo.fileName) - 1] = '\0';
    newFileInfo.fileStat = file_info;
    newFileInfo.time = now;

    // Записываем информацию о файле в архив
    if (write(archive, &newFileInfo, sizeof(fileInfo)) != sizeof(fileInfo)) {
        printf("Ошибка записи информации о файле %s в архив\n", filename);
        close(archive);
        close(file);
        return 1;
    }

    char *bufFile = malloc(file_info.st_size);
    if (read(file, bufFile, file_info.st_size) != file_info.st_size) {
        printf("Ошибка чтения содержимого файла %s\n", filename);
        free(bufFile);
        close(archive);
        close(file);
        return 1;
    }

    if (write(archive, bufFile, file_info.st_size) != file_info.st_size) {
        printf("Ошибка записи содержимого файла %s в архив\n", filename);
        free(bufFile);
        close(archive);
        close(file);
        return 1;
    }

    free(bufFile);
    close(archive);
    close(file);
    return 0;
}

int deleteFile(char *archname, char *filename) {
    createArchive(".bufArch");
    int bufArch = open(".bufArch", O_WRONLY); 
    int archive = open(archname, O_RDONLY);

    bool fileExist = true;
    if (archive == -1) return 1;

    // Чтение архива и копирование файлов, кроме удаляемого
    while (1) {
        fileInfo *a = malloc(sizeof(fileInfo));
        if (!read(archive, a, sizeof(fileInfo))) break;

        lseek(archive, sizeof(fileInfo), SEEK_CUR);
        char *bufFile = malloc(a->fileStat.st_size);
        read(archive, bufFile, a->fileStat.st_size);

        if (strcmp(a->fileName, filename)) {
            write(bufArch, a, sizeof(fileInfo));
            lseek(bufArch, sizeof(fileInfo), SEEK_CUR);
            write(bufArch, bufFile, a->fileStat.st_size);
        } else {
            fileExist = false; // Удаляем файл
        }

        free(bufFile);
        free(a);
    }

    close(archive);
    close(bufArch);

    remove(archname); 
    rename(".bufArch", archname); // Переименовываем временный архив в основной

    return fileExist; // Возвращаем успех или ошибку
}

int extractFile(char *archname, char *filename) {
    int archive = open(archname, O_RDONLY);
    if (archive == -1) return 1;

    fileInfo a;
    bool fileExist = false;

    // Ищем файл в архиве
    while (read(archive, &a, sizeof(fileInfo)) == sizeof(fileInfo)) {
        if (strcmp(a.fileName, filename) == 0) {
            fileExist = true;
            break;
        }
        lseek(archive, a.fileStat.st_size, SEEK_CUR);
    }

    if (!fileExist) {
        printf("Файл %s не найден в архиве\n", filename);
        close(archive);
        return 1;
    }

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    char flag;
    // Проверяем, существует ли файл на диске
    if (access(filename, F_OK) != -1) {
        printf("Файл %s будет перезаписан. Вы хотите продолжить [y/n]?\n", filename);
        scanf(" %c", &flag);
        if (flag == 'n') {
            close(archive);
            return 2;
        }
    }

    // Создаем новый файл и записываем в него данные
    int file = creat(filename, mode);
    if (file == -1) {
        printf("Ошибка создания файла %s\n", filename);
        close(archive);
        return 1;
    }

    char *bufFile = malloc(a.fileStat.st_size);
    if (read(archive, bufFile, a.fileStat.st_size) != a.fileStat.st_size) {
        printf("Ошибка извлечения содержимого файла %s\n", filename);
        free(bufFile);
        close(archive);
        close(file);
        return 1;
    }

    if (write(file, bufFile, a.fileStat.st_size) != a.fileStat.st_size) {
        printf("Ошибка записи содержимого файла %s на диск\n", filename);
        free(bufFile);
        close(archive);
        close(file);
        return 1;
    }

    free(bufFile);
    close(file);
    close(archive);
    chmod(filename, a.fileStat.st_mode); // Восстанавливаем права доступа к файлу
    return 0;
}

void showArchiveState(char *archname) {
	int archive = open(archname, O_RDONLY);
	if (archive == -1) {
		printf("Архив %s не существует\n", archname);
		return;
	}
	size_t count = 0;
	while (1) {
		fileInfo a;
		if (!read(archive, &a, sizeof(fileInfo))) break; // Чтение информации о файле
		// Вывод информации о файле
		printf("Имя файла: %s\n", a.fileName);
		printf("Вес файла: %ld\n", a.fileStat.st_size);
		printf("Время добавления в архив: %s\n", ctime(&a.time));
		lseek(archive, a.fileStat.st_size, SEEK_CUR); // Пропуск содержимого файла
		count++;
	}
	printf("Всего файлов в архиве: %ld\n", count);
	close(archive);
}

int main(int argc, char *argv[]) {
    if (argc < 3) { // Проверка на минимальное количество аргументов
        printf("Использование:\n");
        printf("./archiver archive_name -i file_name\n");
        printf("./archiver archive_name -e file_name\n");
        printf("./archiver archive_name -d file_name\n");
        printf("./archiver archive_name -s\n");
        printf("./archiver archive_name -h\n");
        return 0;
    }

    char *archive = argv[1]; 
    char *flag = argv[2]; 

    if (strcmp(flag, "-i") == 0) {
        for (int i = 3; i < argc; i++) {
            addFile(archive, argv[i]);
        }
    } else if (strcmp(flag, "-e") == 0) {
        extractFile(archive, argv[3]);
    } else if (strcmp(flag, "-d") == 0) {
        deleteFile(archive, argv[3]);
    } else if (strcmp(flag, "-s") == 0) {
        showArchiveState(archive);
    } else if (strcmp(flag, "-h") == 0) {
        printf("Справка по использованию программы:\n");
        printf("Добавить файл: ./archiver archive_name -i file_name\n");
        printf("Извлечь файл:  ./archiver archive_name -e file_name\n");
        printf("Удалить файл:  ./archiver archive_name -d file_name\n");
        printf("Показать состояние архива: ./archiver archive_name -s\n");
    }
    return 0;
}
