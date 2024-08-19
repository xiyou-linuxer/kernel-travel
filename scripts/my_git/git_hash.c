#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <string.h>

unsigned int simple_hash(const unsigned char *data, size_t len) {
	unsigned int hash = 0;
	for (size_t i = 0; i < len; ++i) {
		hash = hash * 31 + data[i];  // 31 是一个常用的哈希因子
	}
	return hash;
}

void init() {
	mkdir(".git", 0755);
	mkdir(".git/objects", 0755);
	mkdir(".git/refs", 0755);
	FILE *head = fopen(".git/HEAD", "w");
	fprintf(head, "refs/heads/master");
	fclose(head);
	printf("Initialized empty Git repository.\n");
}

void hash_object(const char *file_path) {
	FILE *file = fopen(file_path, "rb");
	if (!file) {
		printf("Error opening file.\n");
		return;
	}

	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	unsigned char *buffer = malloc(file_size);
	fread(buffer, 1, file_size, file);

	unsigned int hash = simple_hash(buffer, file_size);

	char object_path[128];
	sprintf(object_path, ".git/objects/%08x", hash); // 以8位十六进制表示

	FILE *out = fopen(object_path, "wb");
	fwrite(buffer, 1, file_size, out);

	printf("Saved object with hash: %08x\n", hash);

	fclose(file);
	fclose(out);
	free(buffer);
}

void commit(const char *message) {
	FILE *index_file = fopen(".git/index", "r");
	if (!index_file) {
		printf("No files added to commit.\n");
		return;
	}

	char line[256];
	memset(line, 0, sizeof(line));
	char commit_content[4096] = "";
	while (fgets(line, sizeof(line), index_file)) {
		strcat(commit_content, line);
	}
	fclose(index_file);

	strcat(commit_content, message);
	unsigned int commit_hash = simple_hash((unsigned char *)commit_content, strlen(commit_content));

	char commit_path[128];
	sprintf(commit_path, ".git/objects/%08x", commit_hash);
	FILE *commit_file = fopen(commit_path, "w");
	if (!commit_file) {
		perror("Failed to open commit file");
		return;
	}
	fprintf(commit_file, "%s", commit_content);
	fclose(commit_file);

	FILE *head = fopen(".git/refs/heads/master", "a");
	if (!head) {
		mkdir(".git/refs/heads", 0755);
		head = fopen(".git/refs/heads/master", "a");
	}
	fprintf(head, "%08x\n", commit_hash);
	fclose(head);

	printf("Committed changes with hash: %08x\n", commit_hash);

	// Clear the index file
	index_file = fopen(".git/index", "w");
	if (!index_file) {
		perror("Failed to clear index file");
		return;
	}
	fclose(index_file);
}



void log_history() {
	FILE *head = fopen(".git/refs/heads/master", "r");
	if (!head) {
		printf("No commits found.\n");
		return;
	}

	char commit_hash[9];
	while (fgets(commit_hash, sizeof(commit_hash), head)) {
		char commit_path[128];
		sprintf(commit_path, ".git/objects/%s", commit_hash);
		commit_hash[strcspn(commit_hash, "\n")] = 0;  // 移除换行符

		FILE *commit_file = fopen(commit_path, "r");
		if (commit_file) {
			char commit_content[4096];
			fread(commit_content, 1, sizeof(commit_content), commit_file);
			printf("Commit %s:\n%s\n", commit_hash, commit_content);
			fclose(commit_file);
		}
	}
	fclose(head);
}

void reflog() {
	FILE *head = fopen(".git/refs/heads/master", "r");
	if (!head) {
		printf("No commits found.\n");
		return;
	}

	char commit_hash[9];
	while (fgets(commit_hash, sizeof(commit_hash), head)) {
		printf("Commit: %s", commit_hash);  // 直接输出 commit hash
	}

	fclose(head);
}


void checkout(const char *commit_hash) {
	char object_path[128];
	sprintf(object_path, ".git/objects/%s", commit_hash);

	FILE *commit_file = fopen(object_path, "r");
	if (!commit_file) {
		printf("Commit not found: %s\n", commit_hash);
		return;
	}

	char commit_content[4096];
	fread(commit_content, 1, sizeof(commit_content), commit_file);
	fclose(commit_file);

	char *line = strtok(commit_content, "\n");
	while (line != NULL) {
		if (strstr(line, " ") != NULL) {
			char *hash = strtok(line, " ");
			char *file_path = strtok(NULL, " ");
			char file_object_path[128];
			sprintf(file_object_path, ".git/objects/%s", hash);

			FILE *file_object = fopen(file_object_path, "rb");
			if (file_object) {
				FILE *work_file = fopen(file_path, "wb");

				fseek(file_object, 0, SEEK_END);
				long size = ftell(file_object);
				fseek(file_object, 0, SEEK_SET);

				unsigned char *buffer = malloc(size);
				fread(buffer, 1, size, file_object);

				fwrite(buffer, 1, size, work_file);

				fclose(file_object);
				fclose(work_file);
				free(buffer);
			}
		}
		line = strtok(NULL, "\n");
	}

	printf("Checked out to commit: %s\n", commit_hash);
}


void add(const char *file_path) {
	FILE *file = fopen(file_path, "rb");
	if (!file) {
		printf("Error opening file.\n");
		return;
	}

	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	unsigned char *buffer = malloc(file_size);
	fread(buffer, 1, file_size, file);

	unsigned int hash = simple_hash(buffer, file_size);

	char object_path[128];
	sprintf(object_path, ".git/objects/%08x", hash);
	FILE *object_file = fopen(object_path, "wb");
	if (!object_file) {
		perror("Failed to open object file");
		fclose(file);
		free(buffer);
		return;
	}
	fwrite(buffer, 1, file_size, object_file);
	fclose(object_file);

	FILE *index_file = fopen(".git/index", "a");
	fprintf(index_file, "%08x %s\n", hash, file_path);
	fclose(index_file);

	printf("Added file %s with hash: %08x\n", file_path, hash);

	fclose(file);
	free(buffer);
}


int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Usage: %s <command>\n", argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "init") == 0) {
		init();
	} else if (strcmp(argv[1], "add") == 0) {
		if (argc < 3) {
			printf("Usage: %s add <file>\n", argv[0]);
			return 1;
		}
		add(argv[2]);
	} else if (!strcmp(argv[1], "commit") && !strcmp(argv[2], "-m")) {
		if (argc < 4) {
			printf("Usage: %s commit <message>\n", argv[0]);
			return 1;
		}
		commit(argv[3]);
	} else if (strcmp(argv[1], "log") == 0) {
		log_history();
	} else if (strcmp(argv[1], "reflog") == 0) {
		reflog();
	}else if (strcmp(argv[1], "checkout") == 0) {
		checkout(argv[2]);
	} else {
		printf("Unknown command: %s\n", argv[1]);
	}

	return 0;
}

