#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char ** argv) {
	if (argc < 2) {
		printf("Provide file\n");
		return 1;
	}

	parser_init();
	char * buf = NULL;
	{
		FILE * fp = fopen(argv[1], "rb");
		if (fp == NULL) {
			printf("File %s does not exist\n", argv[1]);
			return 2;
		}

		unsigned long long int length;
		fseek(fp, 0L, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0L, SEEK_SET);

		buf = malloc(length + 1);
		if (buf == NULL) {
			printf("Failed to allocate buffer of size %llu\n", length + 1);
			return 3;
		}
		buf[length] = '\0';
		if (fread(buf, length, 1, fp) != 1) {
			printf("Failed to read from file %s of size %llu\n", argv[1], length);
			free(buf);
			fclose(fp);
			return 4;
		}
		fclose(fp);
	}
	char * p = buf;
	parse(p, &p);
	parser_deinit();

	free(buf);

	return 0;
}
