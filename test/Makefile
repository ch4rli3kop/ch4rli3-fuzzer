all:
	clang -g -fsanitize-coverage=trace-pc-guard test.c -c
	clang coverage.c test.o -fsanitize-coverage=trace-pc-guard -o test
