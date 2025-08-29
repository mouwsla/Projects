#include "game.h"

int main() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, 1);
  nodelay(stdscr, 1);
  curs_set(0);
  painting();

  game_start();
  int game_on = 1;

  while (game_on) {
    timeout(10);
    int ch = getch();

    GameData_t *state = get_game_state();
    Info_t info = update_game_state(state);

    print_tetris(state->status, state->victory_flag, info);
    free_old_info_and_field(&info);

    if (state->status == GAME_OVER || state->victory_flag == 1) {
      while (ch != 's' && ch != TERMINATE_BUTTON) {
        ch = getch();
      }
      game_start();
    }

    action_buttoms(state, get_actions(ch));
    game_on = state->game_on_flag;
  }

  endwin();
  return 0;
}
