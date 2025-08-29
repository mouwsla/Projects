#include "interface.h"

void painting() {
  start_color();
  init_pair(1, COLOR_WHITE, COLOR_BLACK);
  init_pair(2, COLOR_CYAN, COLOR_BLACK);
  init_pair(3, COLOR_BLACK, COLOR_YELLOW);
  init_pair(4, COLOR_BLACK, COLOR_RED);
  init_pair(5, COLOR_WHITE, COLOR_BLACK);
  init_pair(6, COLOR_BLACK, COLOR_GREEN);
  init_pair(7, COLOR_BLACK, COLOR_CYAN);
  init_pair(8, COLOR_BLACK, COLOR_BLUE);
  init_pair(9, COLOR_BLACK, COLOR_YELLOW);
  init_pair(10, COLOR_BLACK, COLOR_WHITE);
  init_pair(11, COLOR_BLACK, COLOR_GREEN);
  init_pair(12, COLOR_BLACK, COLOR_MAGENTA);
  init_pair(13, COLOR_BLACK, COLOR_RED);
}

void print_tetris(int status, int win, Info_t info) {
  clear();
  refresh();

  int screen_h, screen_w;
  getmaxyx(stdscr, screen_h, screen_w);

  int total_width = GAME_WIN_WIDTH + INFO_PANEL_WIDTH + 3;
  int start_y = (screen_h - GAME_WIN_HEIGHT) / 2;
  int start_x = (screen_w - total_width) / 2;

  WINDOW *game_window = NULL;
  if (!info.pause) {
    game_window = print_game_field(info, start_y, start_x);
    wrefresh(game_window);
  }

  if (status == GAME_INIT)
    print_start_menu(start_y, start_x);
  else if (info.pause)
    print_pause_menu(start_y, start_x);
  else if (win)
    print_win(info, start_y, start_x);
  else if (status == GAME_OVER)
    print_game_over(info, start_y, start_x);

  WINDOW *info_window =
      print_game_info(info, start_y, start_x + GAME_WIN_WIDTH + 2);
  wrefresh(info_window);

  if (game_window) delwin(game_window);
  if (info_window) delwin(info_window);
}

void print_start_menu(int base_y, int base_x) {
  int y = base_y + (GAME_WIN_HEIGHT - MENU_START_HEIGHT) / 2;
  int x = base_x + (GAME_WIN_WIDTH - MENU_START_WIDTH) / 2;

  WINDOW *win = newwin(MENU_START_HEIGHT, MENU_START_WIDTH, y, x);
  box(win, 0, 0);

  wattron(win, COLOR_PAIR(1));
  mvwprintw(win, 2, (MENU_START_WIDTH - 18) / 2, "   -> TETRIS <-");
  wattroff(win, COLOR_PAIR(1));

  wattron(win, COLOR_PAIR(2));
  mvwprintw(win, 4, (MENU_START_WIDTH - 20) / 2, "Press 'S' to start");
  wattroff(win, COLOR_PAIR(2));

  wrefresh(win);
  delwin(win);
}

void print_pause_menu(int base_y, int base_x) {
  int y = base_y + (GAME_WIN_HEIGHT - MENU_PAUSE_HEIGHT) / 2;
  int x = base_x + (GAME_WIN_WIDTH - MENU_PAUSE_WIDTH) / 2;

  WINDOW *win = newwin(MENU_PAUSE_HEIGHT, MENU_PAUSE_WIDTH, y, x);
  box(win, 0, 0);
  wbkgd(win, COLOR_PAIR(3));

  mvwprintw(win, MENU_PAUSE_HEIGHT / 2, (MENU_PAUSE_WIDTH - 6) / 2, "Paused");

  wrefresh(win);
  delwin(win);
}

void print_win(Info_t info, int base_y, int base_x) {
  int y = base_y + (GAME_WIN_HEIGHT - MENU_END_HEIGHT) / 2;
  int x = base_x + (GAME_WIN_WIDTH - MENU_END_WIDTH) / 2;

  WINDOW *win = newwin(MENU_END_HEIGHT, MENU_END_WIDTH, y, x);
  box(win, 0, 0);
  wbkgd(win, COLOR_PAIR(6));

  mvwprintw(win, 2, (MENU_END_WIDTH - 9) / 2, "YOU WIN !");
  mvwprintw(win, 4, (MENU_END_WIDTH - 12) / 2, "Score:  %d", info.score);

  wrefresh(win);
  delwin(win);
}

void print_game_over(Info_t info, int base_y, int base_x) {
  int y = base_y + (GAME_WIN_HEIGHT - MENU_END_HEIGHT) / 2;
  int x = base_x + (GAME_WIN_WIDTH - MENU_END_WIDTH) / 2;

  WINDOW *win = newwin(MENU_END_HEIGHT, MENU_END_WIDTH, y, x);
  box(win, 0, 0);
  wbkgd(win, COLOR_PAIR(4));

  mvwprintw(win, 2, (MENU_END_WIDTH - 10) / 2, "GAME OVER");
  mvwprintw(win, 4, (MENU_END_WIDTH - 15) / 2, "Score:  %d", info.score);
  mvwprintw(win, 5, (MENU_END_WIDTH - 15) / 2, "Level:  %d", info.level);

  wrefresh(win);
  delwin(win);
}

WINDOW *print_game_field(Info_t info, int start_y, int start_x) {
  WINDOW *game_window =
      newwin(GAME_WIN_HEIGHT, GAME_WIN_WIDTH, start_y, start_x);
  box(game_window, 0, 0);
  keypad(game_window, 1);
  nodelay(game_window, 1);

  mvwprintw(game_window, 0, (GAME_WIN_WIDTH - 6) / 2, "Tetris");

  for (int i = 0; i < FIELD_H; i++) {
    for (int j = 0; j < FIELD_W; j++) {
      if (info.field[i][j] >= 7 && info.field[i][j] <= 13) {
        wattron(game_window, COLOR_PAIR(info.field[i][j]));
        mvwprintw(game_window, i + 1, 3 * j + 1, "   ");
        wattroff(game_window, COLOR_PAIR(info.field[i][j]));
      } else {
        wattron(game_window, COLOR_PAIR(1));
        mvwprintw(game_window, i + 1, 3 * j + 1, "   ");
        wattroff(game_window, COLOR_PAIR(1));
      }
    }
  }

  return game_window;
}

WINDOW *print_game_info(Info_t info, int start_y, int start_x) {
  WINDOW *info_window =
      newwin(GAME_WIN_HEIGHT, INFO_PANEL_WIDTH, start_y, start_x);
  box(info_window, 0, 0);

  mvwprintw(info_window, 0, (INFO_PANEL_WIDTH - 9) / 2, "Game_Info");

  print_next_figure(info_window, info);

  mvwprintw(info_window, 8, 2, "High score:  %d", info.high_score);
  mvwprintw(info_window, 11, 2, "Score:       %d", info.score);
  mvwprintw(info_window, 14, 2, "Level:       %d", info.level);
  mvwprintw(info_window, 17, 2, "Speed:       %d", info.speed);

  return info_window;
}

void print_next_figure(WINDOW *w, Info_t info) {
  mvwprintw(w, 2, 2, "Next");
  for (int i = 0; i < info.next_size; i++) {
    for (int j = 0; j < info.next_size; j++) {
      if (info.next[i][j] == 1) {
        wattron(w, COLOR_PAIR(info.next_color));
        mvwprintw(w, i + 4, j * 3 + 4, "   ");
        wattroff(w, COLOR_PAIR(info.next_color));
      }
    }
  }
}
