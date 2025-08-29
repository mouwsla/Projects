#include "test.h"

START_TEST(test_init_score_zero) {
  game_start();
  GameData_t *state = get_game_state();
  ck_assert_int_eq(state->score, 0);
  finish_game(state);
}
END_TEST

START_TEST(test_init_level_one) {
  game_start();
  GameData_t *state = get_game_state();
  ck_assert_int_eq(state->level, 1);
  finish_game(state);
}
END_TEST

START_TEST(test_pause_toggle) {
  game_start();
  GameData_t *state = get_game_state();
  state->status = GAME_MOVING;

  action_buttoms(state, Pause);
  ck_assert_int_eq(state->is_paused_flag, 1);

  action_buttoms(state, Pause);
  ck_assert_int_eq(state->is_paused_flag, 0);

  finish_game(state);
}
END_TEST

START_TEST(test_spawn_figure) {
  game_start();
  GameData_t *state = get_game_state();
  int old_size = state->next_block_size;
  spawn_figure(state);
  ck_assert_int_eq(state->block_size, old_size);
  finish_game(state);
}
END_TEST

START_TEST(test_move_right) {
  game_start();
  GameData_t *state = get_game_state();
  spawn_figure(state);
  int old_x = state->coord_x;
  move_right(state);
  ck_assert_int_ge(state->coord_x, old_x);
  finish_game(state);
}
END_TEST

START_TEST(test_move_left) {
  game_start();
  GameData_t *state = get_game_state();
  spawn_figure(state);
  int old_x = state->coord_x;
  move_left(state);
  ck_assert_int_le(state->coord_x, old_x);
  finish_game(state);
}
END_TEST

START_TEST(test_attach_figure_sets_field) {
  game_start();
  GameData_t *state = get_game_state();
  spawn_figure(state);
  state->coord_y = FIELD_H - state->block_size;
  attach_figure(state);
  int found = 0;
  for (int i = 0; i < FIELD_H; ++i) {
    for (int j = 0; j < FIELD_W; ++j) {
      if (state->field[i][j] != 0) found = 1;
    }
  }
  ck_assert_int_eq(found, 1);
  finish_game(state);
}
END_TEST

START_TEST(test_process_full_lines_no_change) {
  game_start();
  GameData_t *state = get_game_state();
  int score_before = state->score;
  process_full_lines(state);
  ck_assert_int_eq(state->score, score_before);
  finish_game(state);
}
END_TEST

START_TEST(test_process_full_lines_one_line) {
  game_start();
  GameData_t *state = get_game_state();
  for (int j = 0; j < FIELD_W; j++) {
    state->field[FIELD_H - 1][j] = 1;
  }
  process_full_lines(state);
  ck_assert_int_eq(state->score, 100);
  finish_game(state);
}
END_TEST

Suite *tetris_suite(void) {
  Suite *s = suite_create("Tetris");
  TCase *tc = tcase_create("Core");

  tcase_add_test(tc, test_init_score_zero);
  tcase_add_test(tc, test_init_level_one);
  tcase_add_test(tc, test_pause_toggle);
  tcase_add_test(tc, test_spawn_figure);
  tcase_add_test(tc, test_move_right);
  tcase_add_test(tc, test_move_left);
  tcase_add_test(tc, test_attach_figure_sets_field);
  tcase_add_test(tc, test_process_full_lines_no_change);
  tcase_add_test(tc, test_process_full_lines_one_line);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite *s = tetris_suite();
  SRunner *sr = srunner_create(s);
  srunner_set_fork_status(sr, CK_NOFORK);
  srunner_run_all(sr, CK_VERBOSE);
  int number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? 0 : 1;
}
