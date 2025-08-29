CC := gcc
CFLAGS := -Wall -Wextra -Werror -std=c11

SRC_DIR := brick_game/tetris gui/cli
CORE_FILES := brick_game/tetris/tetris.c gui/cli/interface.c game.c
TEST_FILES := test/test.c brick_game/tetris/tetris.c gui/cli/interface.c
GCOV_FLAGS := --coverage -fprofile-arcs -ftest-coverage

BUILD_DIR := build
TEST_DIR := test
GCOV_DIR := gcov
REPORT_DIR := report
DIST_DIR := s21_Tetris

OS := $(shell uname)
ifeq ($(OS),Darwin)
	OPEN_CMD := open
	PLATFORM_FLAGS := -lncurses -lcheck
else
	OPEN_CMD := xdg-open
	PLATFORM_FLAGS := -lncurses -lcheck -lm -lsubunit
endif

.PHONY: all install uninstall test gcov_report clean dist dvi clang_format clang_check

all: install

install:
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CORE_FILES) -o $(BUILD_DIR)/Tetris $(PLATFORM_FLAGS)
	echo 0 > $(BUILD_DIR)/mvp.txt

uninstall:
	rm -rf $(BUILD_DIR)

test:
	echo 0 > $(TEST_DIR)/mvp.txt
	$(CC) $(CFLAGS) $(TEST_FILES) -o $(TEST_DIR)/test $(PLATFORM_FLAGS)
	cd $(TEST_DIR) && ./test

gcov_report: clean
	mkdir -p $(GCOV_DIR) $(REPORT_DIR)
	echo 0 > $(GCOV_DIR)/mvp.txt
	$(CC) $(CFLAGS) $(GCOV_FLAGS) $(TEST_FILES) -o $(GCOV_DIR)/gcov_test $(PLATFORM_FLAGS)
	cd $(GCOV_DIR) && ./gcov_test
	gcovr -r . --filter='test/test.c' --html --html-details -o $(REPORT_DIR)/coverage.html
	$(OPEN_CMD) $(REPORT_DIR)/coverage.html

dvi:
	$(OPEN_CMD) Documentation.html

dist:
	mkdir -p $(DIST_DIR)
	cp -r brick_game gui test *.c *.h Documentation.html $(DIST_DIR)
	tar -czf $(DIST_DIR).tar.gz $(DIST_DIR)
	rm -rf $(DIST_DIR)

clang_format:
	cp ../materials/linters/.clang-format .clang-format
	clang-format -i *.c *.h \
	              brick_game/tetris/*.c brick_game/tetris/*.h \
	              gui/cli/*.c gui/cli/*.h \
	              test/*.c test/*.h
	rm -f .clang-format

clang_check:
	cp ../materials/linters/.clang-format .clang-format
	clang-format -n *.c *.h \
	              brick_game/tetris/*.c brick_game/tetris/*.h \
	              gui/cli/*.c gui/cli/*.h \
	              test/*.c test/*.h
	rm -f .clang-format

clean:
	rm -rf $(BUILD_DIR) $(GCOV_DIR) $(REPORT_DIR) *.gcno *.gcda
	rm -f $(DIST_DIR).tar.gz
	cd $(TEST_DIR) && rm -f test *.txt
