#define _GNU_SOURCE

#include <ncurses.h>
#include <stdio.h>
#include <unistd.h>

#define COLUMNS 80
#define STRINGS 25

void init_ncurses();
void input_matrix(int matrix[STRINGS][COLUMNS]);
int neighbours(int matrix[STRINGS][COLUMNS], int y, int x);
int is_alive(int neighbours, int state);
void render_matrix(int firstMatrix[STRINGS][COLUMNS], int secondMatrix[STRINGS][COLUMNS]);
void replace_matrix(int firstMatrix[STRINGS][COLUMNS], int secondMatrix[STRINGS][COLUMNS]);
void count_living_cells(int matrix[STRINGS][COLUMNS], int *flag);
void control(char controlCharacter, int *speed, int *flag);

int main() {
    int firstMatrix[STRINGS][COLUMNS], secondMatrix[STRINGS][COLUMNS];
    int speed = 1000, flag = 0, generation = 0;
    input_matrix(firstMatrix);
    if (freopen("/dev/tty", "r", stdin)) initscr();
    init_ncurses();
    while (flag != 1) {
        char controlCharacter = getch();
        control(controlCharacter, &speed, &flag);
        count_living_cells(firstMatrix, &flag);
        usleep(speed * 200);
        clear();
        render_matrix(firstMatrix, secondMatrix);
        replace_matrix(firstMatrix, secondMatrix);
        printw("Generation: %d\n", generation);
        printw("Control buttons:\nSpeed up - click \"+\"\nSlow down - click \"-\"\nExit - click \"q\" or \"Q\"\n");
        generation++;
        refresh();
    }
    endwin();
    return 0;
}

void init_ncurses() {
    nodelay(stdscr, true);
    curs_set(0);
    noecho();
    cbreak();
}

void input_matrix(int matrix[STRINGS][COLUMNS]) {
    for (int i = 0; i < STRINGS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            scanf("%d", &matrix[i][j]);
        }
    }
}

void render_matrix(int firstMatrix[STRINGS][COLUMNS], int secondMatrix[STRINGS][COLUMNS]) {
    for (int i = 0; i < STRINGS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            secondMatrix[i][j] = is_alive(neighbours(firstMatrix, i, j), firstMatrix[i][j]);
            if (secondMatrix[i][j] == 1) {
                printw("#");
            } else {
                printw(" ");
            }
        }
        printw("\n");
    }
}

void control(char controlCharacter, int *speed, int *flag) {
    if (controlCharacter == '+' && *speed > 100) {
        *speed -= 100;
    } else if (controlCharacter == '-' && *speed <= 20000) {
        *speed += 100;
    } else if (controlCharacter == 'q' || controlCharacter == 'Q') {
        *flag = 1;
    }
}

int neighbours(int matrix[STRINGS][COLUMNS], int y, int x) {
    int neighboursCount = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i != 0 || j != 0) {
                int ny = (y + i + STRINGS) % STRINGS;
                int nx = (x + j + COLUMNS) % COLUMNS;
                neighboursCount += matrix[ny][nx];
            }
        }
    }
    return neighboursCount;
}

int is_alive(int neighbours, int state) {
    int status = -1;
    if ((state == 0 && neighbours == 3) || (state == 1 && neighbours == 2) ||
        (state == 1 && neighbours == 3)) {
        status = 1;
    } else if ((state == 1 && neighbours < 2) || (state == 1 && neighbours > 3)) {
        status = 0;
    } else {
        status = 0;
    }
    return status;
}

void replace_matrix(int firstMatrix[STRINGS][COLUMNS], int secondMatrix[STRINGS][COLUMNS]) {
    for (int i = 0; i < STRINGS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            firstMatrix[i][j] = secondMatrix[i][j];
        }
    }
}

void count_living_cells(int matrix[STRINGS][COLUMNS], int *flag) {
    int sum = 0;
    for (int i = 0; i < STRINGS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            sum += matrix[i][j];
        }
    }
    if (sum == 0) *flag = 1;
}
