#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

// Структура для хранения информации о файле
typedef struct {
	struct stat fileStat; // Информация о файле (размер, права доступа и т.д.)
	char fileName[20];    // Имя файла
	time_t time;          // Время добавления файла в архив
} fileInfo;


int createArchive(char *filename) {
	int archive = open(filename, O_WRONLY);
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // Права доступа

	if (archive == -1) { archive = creat(filename, mode); }
	close(archive);
	return (archive == -1) ? 1 : 0;
}

int readArchive(char *archname) {
	int archive = open(archname, O_RDONLY);
	if (archive == -1) return 1;

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
	return 0;
}

int addFile(char *archname, char *filename) {
    if (strcmp(archname, filename) == 0) {
        printf("Архив не может архивировать сам себя\n");
        return 1;
    }

    int archive = open(archname, O_RDONLY);
    if (archive == -1) return 1;

    int file = open(filename, O_RDONLY);
    if (file == -1) return 1;

    bool fileExist = false;
    fileInfo a;

    // Проверка на существование файла в архиве
    while (read(archive, &a, sizeof(fileInfo)) == sizeof(fileInfo)) {
        if (strcmp(a.fileName, filename) == 0) {
            fileExist = true;
            break;
        }
        lseek(archive, a.fileStat.st_size, SEEK_CUR); // Пропуск содержимого файла
    }
    close(archive);

    if (fileExist) {
        printf("Файл %s уже находится в архиве\n", filename);
        close(file);
        return 1;
    }

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
	// Создание временного архива для копирования файлов
	createArchive(".bufArch");

	int bufArch = open(".bufArch", O_WRONLY);
	int archive = open(archname, O_RDONLY);

	bool fileExist = 1;
	if (archive == -1) return 1;

	while (1) {
		char *buf = malloc(sizeof(fileInfo));
		if (!read(archive, buf, sizeof(fileInfo))) break;
		fileInfo *a = (fileInfo*)(buf);

		lseek(archive, sizeof(fileInfo), SEEK_CUR);
		char bufFile[a->fileStat.st_size];
		read(archive, bufFile, a->fileStat.st_size);

		// Если текущий файл не тот, что удаляем, записываем его в временный архив
		if (strcmp(a->fileName, filename)) {
			write(bufArch, a, sizeof(fileInfo));
			lseek(bufArch, sizeof(fileInfo), SEEK_CUR);
			write(bufArch, &bufFile, a->fileStat.st_size);
		} else {
			fileExist = 0;
		}

		free(buf);
	}

	close(archive);
	close(bufArch);

	remove(archname);
	rename(".bufArch", archname);

	return fileExist;
}

// Извлечение
int extractFile(char *archname, char *filename) {
    int archive = open(archname, O_RDONLY);
    if (archive == -1) return 1;

    fileInfo a;
    bool fileExist = false;

    // Поиск файла в архиве
    while (read(archive, &a, sizeof(fileInfo)) == sizeof(fileInfo)) {
        if (strcmp(a.fileName, filename) == 0) {
            fileExist = true;
            break;
        }
        lseek(archive, a.fileStat.st_size, SEEK_CUR); // Пропуск содержимого файла
    }

    if (!fileExist) {
        printf("Файл %s не найден в архиве\n", filename);
        close(archive);
        return 1;
    }

    // Проверка на перезапись, если файл существует
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    char flag;
    if (access(filename, F_OK) != -1) {
        printf("Файл %s будет перезаписан. Вы хотите продолжить [y/n]?\n", filename);
        scanf(" %c", &flag);
        if (flag == 'n') {
            close(archive);
            return 2;
        }
    }

    // Создание нового файла для извлечения
    int file = creat(filename, mode);
    if (file == -1) {
        printf("Ошибка создания файла %s\n", filename);
        close(archive);
        return 1;
    }

    // Чтение содержимого из архива и запись его в файл
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

    // Установка прав доступа на извлечённый файл
    chmod(filename, a.fileStat.st_mode);
    return 0;
}

int main(int argc, char *argv[]) {
	bool cFlag = false; // Создать архив
	bool dFlag = false; // Удалить ф
	bool eFlag = false;	// Извлечь ф
	bool iFlag = false; // Добавить ф
	bool hFlag = false; // Вывод помощи
	bool rFlag = false; // Вывод информации архива

	if (argc <= 1) hFlag = true;

	int opt;
	while ((opt = getopt(argc, argv, "cdeihr")) != -1) {
		switch (opt) {
			case 'c': cFlag = true; break;
			case 'd': dFlag = true; break;
			case 'e': eFlag = true; break;
			case 'i': iFlag = true; break;
			case 'h': hFlag = true; break;
			case 'r': rFlag = true; break;
			default: hFlag = true;
		}
	}

	if (hFlag) {
		printf("Примитивный архиватор\n");
		printf("-c создать архив\n  ./archiver -c archive_name\n");
		printf("-d удалить файл из архива\n  ./archiver -d archive_name file_name\n");
		printf("-e извлечь файл из архива\n  ./archiver -e archive_name file_name\n");
		printf("-i добавить файл в архив\n  ./archiver -i archive_name file_name\n");
		printf("-r показать содержимое архива\n  ./archiver -r archive_name\n");
		printf("-h помощь\n");
		return 0;
	}

	// Если установлен флаг создания архива
	if (cFlag) {
		for (; optind < argc; optind++) {
			if (!createArchive(argv[optind])) 
				printf("Архив %s создан\n", argv[optind]);
			else 
				printf("Ошибка создания архива %s\n", argv[optind]);
		}
	// Если установлен флаг добавления файла
	} else if (iFlag) {
		char *archive = argv[optind++];
		for (; optind < argc; optind++) {
			if (!addFile(archive, argv[optind])) 
				printf("Файл %s добавлен в архив %s\n", argv[optind], archive); 
			else 
				printf("Ошибка добавления файла %s\n", argv[optind]);
		}
	// Если установлен флаг удаления файла
	} else if (dFlag) {
		char *archive = argv[optind++];
		for (; optind < argc; optind++) {
			if (!deleteFile(archive, argv[optind])) 
				printf("Файл %s удалён из архива %s\n", argv[optind], archive); 
			else 
				printf("Ошибка удаления файла %s\n", argv[optind]);
		}
	// Если установлен флаг извлечения файла
	} else if (eFlag) {
		char *archive = argv[optind++];
		for (; optind < argc; optind++) {
			size_t state = extractFile(archive, argv[optind]);
			if (state == 0) 
				printf("Файл %s извлечён из архива %s\n", argv[optind], archive); 
			else if (state == 1) 
				printf("Ошибка извлечения файла %s\n", argv[optind]);
			else 
				printf("Файл %s не будет извлечён\n", argv[optind]);
		}
	// Если установлен флаг чтения содержимого архива
	} else if (rFlag) {
		if (readArchive(argv[optind])) 
			printf("Архив %s не существует\n", argv[optind]);
	}
	return 0;
}