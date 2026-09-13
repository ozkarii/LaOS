#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ramdisk.h"

#define COPY_BUFFER_SIZE 65536

typedef struct InputFile {
	const char* source_path;
	const char* ramdisk_path;
	uint32_t size;
} InputFile;

static void print_usage(const char* program) {
	fprintf(stderr, "Usage: %s -o <output> <source-binary> <ramdisk-path>...\n", program);
}

static bool copy_file(FILE* out, const char* path) {
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		return false;
	}

	uint8_t buffer[COPY_BUFFER_SIZE];
	while (true) {
		ssize_t read_bytes = read(fd, buffer, sizeof(buffer));
		if (read_bytes < 0) {
			if (errno == EINTR) {
				continue;
			}
			close(fd);
			return false;
		}

		if (read_bytes == 0) {
			break;
		}

		size_t written = 0;
		while (written < (size_t)read_bytes) {
			size_t chunk = fwrite(buffer + written, 1, (size_t)read_bytes - written, out);
			if (chunk == 0) {
				close(fd);
				return false;
			}
			written += chunk;
		}
	}

	close(fd);
	return true;
}

static bool write_all(FILE* out, const void* data, size_t size) {
	const uint8_t* bytes = (const uint8_t*)data;
	size_t written = 0;

	while (written < size) {
		size_t chunk = fwrite(bytes + written, 1, size - written, out);
		if (chunk == 0) {
			return false;
		}
		written += chunk;
	}

	return true;
}

int main(int argc, char** argv) {
	const char* output_path = NULL;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-o") == 0) {
			if (i + 1 >= argc) {
				print_usage(argv[0]);
				return EXIT_FAILURE;
			}
			output_path = argv[++i];
			continue;
		}

		if (argv[i][0] == '-') {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	if (output_path == NULL) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	size_t positional_count = 0;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-o") == 0) {
			i++;
			continue;
		}

		if (argv[i][0] == '-') {
			continue;
		}

		positional_count++;
	}

	if (positional_count == 0 || (positional_count % 2) != 0) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	size_t file_count = positional_count / 2;

	InputFile* inputs = (InputFile*)calloc(file_count, sizeof(InputFile));
	RamdiskEntry* entries = (RamdiskEntry*)calloc(file_count, sizeof(RamdiskEntry));
	if (inputs == NULL || entries == NULL) {
		free(inputs);
		free(entries);
		return EXIT_FAILURE;
	}

	size_t input_index = 0;
	uint64_t next_offset = sizeof(RamdiskHeader) + file_count * sizeof(RamdiskEntry);

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-o") == 0) {
			i++;
			continue;
		}

		if (argv[i][0] == '-') {
			continue;
		}

		const char* source_path = argv[i++];
		const char* ramdisk_path = argv[i];
		size_t ramdisk_path_len = strlen(ramdisk_path);
		if (ramdisk_path_len >= PATH_NAME_MAX) {
			fprintf(stderr, "ramdisk-gen: path too long: %s\n", ramdisk_path);
			free(inputs);
			free(entries);
			return EXIT_FAILURE;
		}

		struct stat st;
		if (stat(source_path, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0) {
			fprintf(stderr, "ramdisk-gen: failed to stat %s: %s\n", source_path, strerror(errno));
			free(inputs);
			free(entries);
			return EXIT_FAILURE;
		}

		if ((uint64_t)st.st_size > UINT32_MAX) {
			fprintf(stderr, "ramdisk-gen: file too large: %s\n", source_path);
			free(inputs);
			free(entries);
			return EXIT_FAILURE;
		}

		if (next_offset > UINT32_MAX || next_offset + (uint64_t)st.st_size > UINT32_MAX) {
			fprintf(stderr, "ramdisk-gen: image is too large\n");
			free(inputs);
			free(entries);
			return EXIT_FAILURE;
		}

		inputs[input_index].source_path = source_path;
		inputs[input_index].ramdisk_path = ramdisk_path;
		inputs[input_index].size = (uint32_t)st.st_size;

		memset(&entries[input_index], 0, sizeof(RamdiskEntry));
		memcpy(entries[input_index].path, ramdisk_path, ramdisk_path_len + 1);
		entries[input_index].offset = (uint32_t)next_offset;
		entries[input_index].size = (uint32_t)st.st_size;

		next_offset += (uint64_t)st.st_size;
		input_index++;
	}

	RamdiskHeader header;
	memset(&header, 0, sizeof(header));
	memcpy(header.magic, RAMDISK_MAGIC, strlen(RAMDISK_MAGIC) + 1);
	header.file_count = (uint32_t)file_count;
	header.table_offset = (uint32_t)sizeof(RamdiskHeader);
	header.data_offset = (uint32_t)(sizeof(RamdiskHeader) + file_count * sizeof(RamdiskEntry));

	FILE* out = fopen(output_path, "wb");
	if (out == NULL) {
		fprintf(stderr, "ramdisk-gen: failed to open %s: %s\n", output_path, strerror(errno));
		free(inputs);
		free(entries);
		return EXIT_FAILURE;
	}

	bool ok = write_all(out, &header, sizeof(header)) &&
						write_all(out, entries, file_count * sizeof(RamdiskEntry));

	for (size_t i = 0; ok && i < file_count; i++) {
		ok = copy_file(out, inputs[i].source_path);
	}

	if (fclose(out) != 0) {
		ok = false;
	}

	free(inputs);
	free(entries);

	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
