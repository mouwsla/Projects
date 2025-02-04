#include <stdio.h>

// Display cleaning function
void clearScreen() { printf("\033[H\033[J"); }

// Function for moving 1 racket
int rocket_move_1(int RX1, char t) {
    if (t == 'a' || t == 'A') {
        if (RX1 > 2) {
            return RX1 - 1;
        } else {
            return RX1;
        }
    } else if (t == 'z' || t == 'Z') {
        if (RX1 < 23) {
            return RX1 + 1;
        } else {
            return RX1;
        }
    } else {
        return RX1;
    }
}

// Function for moving 2 rackets
int rocket_move_2(int RX2, char t) {
    if (t == 'k' || t == 'K') {
        if (RX2 > 2) {
            return RX2 - 1;
        } else {
            return RX2;
        }
    } else if (t == 'm' || t == 'M') {
        if (RX2 < 23) {
            return RX2 + 1;
        } else {
            return RX2;
        }
    } else {
        return RX2;
    }
}
// Function to display the winner
int Points(int a, int b) {
    if (a == 21) {
        printf("\nCONGRATULATIONS! PLAYER ONE WIN WITH %d POINTS!", a);
        return 1;
    } else if (b == 21) {
        printf("\nCONGRATULATIONS! PLAYER TWO WIN WITH %d POINTS!", b);
        return 1;
    }
    return 0;
}
// Function to display the field, ball and counter
void Map(int a, int b, int b_y, int b_x, int ry_1, int ry_2, int k1, int k2) {
    clearScreen();
    for (int i = 0; i <= b; i++) {
        for (int j = 0; j <= a; j++) {
            if (i == 0 || i == b) {
                printf("-");
            } else if (j == 0 || j == a) {
                printf("|");
            } else if (j == a / 2 - 1) {
                printf("|");
            } else if ((j == 1) && ((i <= ry_1 + 1) && (i >= ry_1 - 1))) {
                printf("#");
            } else if ((j == a - 1) && ((i <= ry_2 + 1) && (i >= ry_2 - 1))) {
                printf("#");
            } else if ((i == b_y) && (j == b_x)) {
                printf("*");
            } else if (j == 37 && i == 1) {
                printf("%d", k1 / 10);
            } else if (j == 38 && i == 1) {
                printf("%d", k1 % 10);
            } else if (j == 40 && i == 1) {
                printf("%d", k2 / 10);
            } else if (j == 41 && i == 1) {
                printf("%d", k2 % 10);
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
    printf("Exit: Control + C\n");
}
int main() {
    int score_1 = 0;
    int score_2 = 0;
    int width = 80;
    int height = 25;
    int b_x = width / 2;
    int b_y = height / 2;
    int r_y_1 = height / 2;
    int r_y_2 = height / 2;
    int axis_updateX = 1;
    int axis_updateY = 1;
    while (1) {
        Map(width, height, b_y, b_x, r_y_1, r_y_2, score_1, score_2);
        b_x += axis_updateX;
        b_y += axis_updateY;
        char ch;
        scanf("%c", &ch);
        if (ch == '\n') {
            scanf("%c", &ch);
        }
        r_y_1 = rocket_move_1(r_y_1, ch);
        r_y_2 = rocket_move_2(r_y_2, ch);
        if (b_x == 1) {
            axis_updateX = -axis_updateX;
            b_x = width / 2;
            b_y = height / 2;
            score_2++;
        } else if (b_x == width - 1) {
            axis_updateX = -axis_updateX;
            b_x = width / 2;
            b_y = height / 2;
            score_1++;
        } else if (b_x == 2 && ((b_y >= r_y_1 - 1) && (b_y <= r_y_1 + 1))) {
            axis_updateX = -axis_updateX;
        } else if (b_x == width - 2 && ((b_y >= r_y_2 - 1) && (b_y <= r_y_2 + 1))) {
            axis_updateX = -axis_updateX;
        }
        if (b_y == 1 || b_y == height - 1) {
            axis_updateY = -axis_updateY;
        }
        if (Points(score_1, score_2)) {
            return 1;
        }
    }
    return 0;
}
