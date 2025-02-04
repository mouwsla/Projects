var1 = gcc
var2 = -Wall -Werror -Wextra -std=c11

all: s21_grep

s21_grep: s21_grep_funcs.o
	$(var1) $(var2) -o ../grep/s21_grep s21_grep.c s21_grep_funcs.o

s21_grep_funcs.o:
	$(var1) $(var2) -c s21_grep_funcs.c -o s21_grep_funcs.o


clean:
	rm -f ../grep/s21_grep
	rm -f ../grep/*.o

rebuild: clean all