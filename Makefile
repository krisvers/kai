all:
	clang $(shell find . -type f -name "*.c") -o kai -Wall -Wextra -Wpedantic
