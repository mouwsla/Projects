var1 = gcc
var2 = -Wall -Werror -Wextra -std=c11

all: s21_cat

s21_cat: func.o 
	$(var1) $(var2) -o ../cat/s21_cat s21_cat.c func.o

func.o:
	$(var1) $(var2) -c func.c -o func.o


clean:
	rm -f ../cat/s21_cat
	rm -f ../cat/*.o


rebuild: clean all

