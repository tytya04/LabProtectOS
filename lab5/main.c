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
	struct stat fileStat; // Информация о файле
	char fileName[20];    // Имя файла
	time_t time;          // Время добавления файла
} fileInfo;

int createArchive(char *filename) {
	int archive = open(filename, O_WRONLY);
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

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
		if (!read(archive, &a, sizeof(fileInfo))) break;

		printf("Имя файла: %s\n", a.fileName);
		printf("Вес файла: %ld\n", a.fileStat.st_size);
		printf("Время добавления в архив: %s\n", ctime(&a.time));
		lseek(archive, a.fileStat.st_size, SEEK_CUR);
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

	while (read(archive, &a, sizeof(fileInfo)) == sizeof(fileInfo)) {
		if (strcmp(a.fileName, filename) == 0) {
			fileExist = true;
			break;
		}
		lseek(archive, a.fileStat.st_size, SEEK_CUR);
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

	while (1) {
		char *buf = malloc(sizeof(fileInfo));
		if (!read(archive, buf, sizeof(fileInfo))) break;
		fileInfo *a = (fileInfo*)(buf);

		lseek(archive, sizeof(fileInfo), SEEK_CUR);
		char bufFile[a->fileStat.st_size];
		read(archive, bufFile, a->fileStat.st_size);

		if (strcmp(a->fileName, filename)) {
			write(bufArch, a, sizeof(fileInfo));
			lseek(bufArch, sizeof(fileInfo), SEEK_CUR);
			write(bufArch, &bufFile, a->fileStat.st_size);
		} else {
			fileExist = false;
		}

		free(buf);
	}

	close(archive);
	close(bufArch);

	remove(archname);
	rename(".bufArch", archname);

	return fileExist;
}

int extractFile(char *archname, char *filename) {
	int archive = open(archname, O_RDONLY);
	if (archive == -1) return 1;

	fileInfo a;
	bool fileExist = false;

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
	if (access(filename, F_OK) != -1) {
		printf("Файл %s будет перезаписан. Вы хотите продолжить [y/n]?\n", filename);
		scanf(" %c", &flag);
		if (flag == 'n') {
			close(archive);
			return 2;
		}
	}

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
	chmod(filename, a.fileStat.st_mode);

	if (deleteFile(archname, filename)) {
		printf("Ошибка удаления файла %s из архива\n", filename);
	}

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
		printf("./archiver archive_name -r\n");
		printf("./archiver archive_name -h\n");
		return 0;
	}

	char *archive = argv[1]; // Название архива
	char *flag = argv[2]; // Флаг операции

	if (strcmp(flag, "-c") == 0) {
		// Создание архива
		if (createArchive(archive) == 0) {
			printf("Архив %s создан\n", archive);
		} else {
			printf("Ошибка создания архива %s\n", archive);
		}
	} else if (strcmp(flag, "-i") == 0) {
		for (int i = 3; i < argc; i++) {
			if (addFile(archive, argv[i]) == 0) {
				printf("Файл %s добавлен в архив %s\n", argv[i], archive);
			} else {
				printf("Ошибка добавления файла %s\n", argv[i]);
			}
		}
	} else if (strcmp(flag, "-d") == 0) {
		for (int i = 3; i < argc; i++) {
			if (deleteFile(archive, argv[i]) == 0) {
				printf("Файл %s удалён из архива %s\n", argv[i], archive);
			} else {
				printf("Ошибка удаления файла %s\n", argv[i]);
			}
		}
	} else if (strcmp(flag, "-e") == 0) {
		for (int i = 3; i < argc; i++) {
			if (extractFile(archive, argv[i]) == 0) {
				printf("Файл %s извлечён из архива %s\n", argv[i], archive);
			} else {
				printf("Ошибка извлечения файла %s\n", argv[i]);
			}
		}
	} else if (strcmp(flag, "-r") == 0) {
		if (readArchive(archive) != 0) {
			printf("Архив %s не существует\n", archive);
		}
	} else if (strcmp(flag, "-h") == 0) {
		printf("Примитивный архиватор\n");
		printf("-c создать архив\n  ./archiver archive_name -c\n");
		printf("-d удалить файл из архива\n  ./archiver archive_name -d file_name\n");
		printf("-e извлечь файл из архива\n  ./archiver archive_name -e file_name\n");
		printf("-i добавить файл в архив\n  ./archiver archive_name -i file_name\n");
		printf("-r показать содержимое архива\n  ./archiver archive_name -r\n");
		printf("-h помощь\n");
	} else {
		printf("Неизвестный флаг: %s\n", flag);
	}

	return 0;
}