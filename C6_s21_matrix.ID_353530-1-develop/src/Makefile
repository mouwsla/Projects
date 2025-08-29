# CC=gcc
# FLAGS=-Wall -Werror -Wextra -std=c11
# MAC_FLAGS=-lcheck -lm
# LINUX_FLAGS=-lcheck -lm -lrt -lpthread -lsubunit
# GCOV_FLAGS=-fprofile-arcs -ftest-coverage

# FILES=$(wildcard s21_*.c)
# OBJ=$(patsubst %o, %o, $(patsubst %.c,%.o,$(FILES)))
# TEST_FILES = $(wildcard tests/test.c)
# USERNAME=$(shell whoami)

# MYLIB = s21_matrix.a

# OS := $(shell uname -s)

# ifeq ($(OS),Darwin)
# 	OPEN=open
# endif
# ifeq ($(OS),Linux)
# 	OPEN=xdg-open
# endif

# all: s21_matrix.a

# .PHONY: all test s21_matrix.a gcov_report clean rebuild

# s21_matrix.a: $(FILES)
# 	$(CC) -c $(FLAGS) $(FILES)
# 	ar rc $@ $(OBJ)
# 	ranlib $@
# 	rm -rf *.o

# test: s21_matrix.a
# ifeq ($(OS),Darwin)
# 	$(CC) $(FLAGS) $(GCOV_FLAGS) $(TEST_FILES) $< -o test $(MAC_FLAGS)
# else
# 	$(CC) $(FLAGS) $(GCOV_FLAGS) $(TEST_FILES) $< -o test $(LINUX_FLAGS)
# endif
# 	./test

# gcov_report: clean
# 	mkdir -p report
# ifeq ($(OS),Darwin)
# 	$(CC) $(FLAGS) $(GCOV_FLAGS) $(FILES) $(TEST_FILES) -o test $(MAC_FLAGS)
# else
# 	$(CC) $(FLAGS) $(GCOV_FLAGS) $(FILES) $(TEST_FILES) -o test $(LINUX_FLAGS)
# endif
# 	./test
# 	gcovr -r . --html --html-details -o report/coverage.html
# 	$(OPEN) report/coverage.html

# style_check:
# 	clang-format -i -style=google tests/*.c *.c *.h
# 	clang-format -n -style=google tests/*.c *.c *.h

# check:
# 	leaks -atExit -- ./tests/test

# clean:
# 	rm -rf *.a *.o *.gcda *.gcno *.gcov test report

# rebuild: clean all


# #--exclude tests/



CC=gcc
FLAGS=-Wall -Werror -Wextra -std=c11
MAC_FLAGS=-lcheck -lm
LINUX_FLAGS=-lcheck -lm -lrt -lpthread -lsubunit
GCOV_FLAGS=-fprofile-arcs -ftest-coverage

FILES=$(wildcard *.c)
OBJ=$(patsubst %o, %o, $(patsubst %.c,%.o,$(FILES)))
TEST_FILES = $(wildcard tests/test*.c)
USERNAME=$(shell whoami)

MYLIB = s21_matrix.a

OS := $(shell uname -s)

ifeq ($(OS),Darwin)
	OPEN=open
endif
ifeq ($(OS),Linux)
	OPEN=xdg-open
endif

all: s21_matrix.a


s21_matrix.a: $(FILES)
	$(CC) -c $(FLAGS) $(FILES)
	ar rc $@ $(OBJ)
	ranlib $@

test: s21_matrix.a
ifeq ($(OS),Darwin)
	$(CC) $(FLAGS) $(GCOV_FLAGS) $(TEST_FILES) $< -o test $(MAC_FLAGS)
else
	$(CC) $(FLAGS) $(GCOV_FLAGS) $(TEST_FILES) $< -o test $(LINUX_FLAGS)
endif
	./test

gcov_report: test
	mkdir -p report
ifeq ($(OS),Darwin)
	$(CC) $(FLAGS) $(GCOV_FLAGS) $(FILES) $(TEST_FILES) -o test $(MAC_FLAGS)
else
	$(CC) $(FLAGS) $(GCOV_FLAGS) $(FILES) $(TEST_FILES) -o test $(LINUX_FLAGS)
endif
	./test
	gcovr -r . --exclude tests/ --html --html-details -o report/coverage.html
	$(OPEN) report/coverage.html

style_check:
	clang-format -i -style=google tests/*.c tests/*.h *.c *.h
	clang-format -n -style=google tests/*.c tests/*.h *.c *.h
	
check:
	leaks -atExit -- ./test

clean:
	rm -rf *.a *.o *.gcda *.gcno *.gcov test report

rebuild: clean all
