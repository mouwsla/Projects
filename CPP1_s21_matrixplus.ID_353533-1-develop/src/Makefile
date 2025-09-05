CC = g++
CFLAGS = -Wall -Wextra -Werror -g -std=c++20
GCOVFLAGS = -fprofile-arcs -ftest-coverage

PATH_TO_TESTS = tests/
EXEC = s21_matrix_oop.a
EXEC_T = unit_tests

COV_REPORT = report

SRC = $(wildcard *.cpp)
OBJ = $(patsubst %.cpp, %.o, $(SRC))
SRC_T = $(wildcard $(PATH_TO_TESTS)*.cpp)
OBJ_T = $(patsubst %.cpp, %.o, $(SRC_T))

OS := $(shell uname)
OS := $(shell uname)
ifeq ($(OS),Darwin)
	OPEN_CMD := open
	TEST_LIBS = -lcheck -lpthread -lm
else
	OPEN_CMD := xdg-open
	TEST_LIBS = -lcheck -lpthread -lm -lgcov
endif


all: $(EXEC) test gcov_report

s21_matrix_oop.a: $(OBJ)
	ar rc $@ $(OBJ)
	ranlib $@

test: $(OBJ_T) $(EXEC)
ifeq ($(OS),Darwin)
	$(CC) $(CFLAGS) $(OBJ_T) $(EXEC) -o $(PATH_TO_TESTS)$(EXEC_T) -lgtest -lgtest_main
else
	$(CC) $(CFLAGS) $(OBJ_T) $(EXEC) -o $(PATH_TO_TESTS)$(EXEC_T) -lgtest -lgtest_main -lpthread
endif
	$(PATH_TO_TESTS)./$(EXEC_T)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

gcov_report: s21_matrix_oop.a
	$(CC) $(CFLAGS) $(GCOVFLAGS) $(PATH_TO_TESTS)*.cpp s21_matrix_oop.a $(TEST_LIBS) -o s21_matrix_oop -lgtest -lgtest_main
	./s21_matrix_oop
	mkdir -p $(COV_REPORT)
	gcovr -r . --html --html-details -o $(COV_REPORT)/index.html
	rm -rf *.gcda *.gcno
	$(OPEN_CMD) $(COV_REPORT)/index.html

format-check:
	@cp ../materials/linters/.clang-format ./
	@clang-format -n *.h *.cpp $(PATH_TO_TESTS)*.cpp #$(PATH_TO_TESTS)*.h
	@rm .clang-format

format:
	@cp ../materials/linters/.clang-format ./
	@clang-format -i *.h *.cpp $(PATH_TO_TESTS)*.cpp #$(PATH_TO_TESTS)*.h
	@rm .clang-format

clean:
	@rm -rf *.o $(EXEC) $(PATH_TO_TESTS)*.o $(PATH_TO_TESTS)$(EXEC_T)
	@rm -rf *.gcda *.gcno *.info *.gcov
	@rm -rf RESULT_VALGRIND.txt gcov_*
	@rm -rf $(COV_REPORT)
	@rm -rf s21_matrix_oop s21_matrix_oop.dSYM

