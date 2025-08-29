#ifndef INTERFACE_H
#define INTERFACE_H

// #define GAME_FIELD_H (FIELD_H + 2)
// #define GAME_FIELD_W (FIELD_W * 3 + 2)

// #define START_MENU_N 8
// #define START_MENU_M 24
// #define PAUSE_MENU_N 5
// #define PAUSE_MENU_M 24
// #define FINISH_GAME_N 10
// #define FINISH_GAME_M 24
// #define GAME_INFO_M 25
// #define CONTROLS_M 24

// #define TOP_MARGIN 0
// #define LEFT_MARGIN 0

#define GAME_WIN_HEIGHT (FIELD_H + 2)
#define GAME_WIN_WIDTH (FIELD_W * 3 + 2)

#define MENU_START_HEIGHT 8
#define MENU_START_WIDTH 24
#define MENU_PAUSE_HEIGHT 5
#define MENU_PAUSE_WIDTH 24
#define MENU_END_HEIGHT 10
#define MENU_END_WIDTH 24

#define INFO_PANEL_WIDTH 25

#include <ncurses.h>

#include "../../game.h"

void painting();
void print_tetris(int status, int win, Info_t info);

void print_start_menu(int y, int x);
void print_pause_menu(int y, int x);
void print_win(Info_t info, int y, int x);
void print_game_over(Info_t info, int y, int x);
WINDOW *print_game_field(Info_t info, int y, int x);
WINDOW *print_game_info(Info_t info, int y, int x);
void print_next_figure(WINDOW *w, Info_t info);

// WINDOW *print_controls();
// WINDOW *print_game_field(Info_t info);
// void print_start_menu();
// void print_pause_menu();
// void print_win(Info_t info);
// void print_game_over(Info_t info);
// WINDOW *print_game_info(Info_t info);
// void print_next_figure(WINDOW *info_window, Info_t info);

#endif