#include "tetris.h"

void game_start() {
  GameData_t *state = get_game_state();
  state->status = GAME_INIT;
  state->speed = SPEED;
  state->score = 0;
  state->level = 1;
  state->coord_x = 4;
  state->coord_y = -4;
  state->is_paused_flag = 0;
  state->victory_flag = 0;
  state->game_on_flag = 1;
  state->time = current_time();
  srand(current_time());
  state->field = create_matrix(FIELD_H, FIELD_W);
  int size = 0, type = 0;
  state->next_block = gen_block(&size, &type);
  state->next_block_color = type + 7;
  state->next_block_size = size;
  state->block = create_matrix(size, size);
  state->block_size = size;
  copy_matrix(state->block, state->next_block, size, size);
  FILE *file = fopen("mvp.txt", "r");
  if (file) {
    fscanf(file, "%d", &state->mvp_score);
    fclose(file);
  }
}

GameData_t *get_game_state() {
  static GameData_t state;
  return &state;
}

Info_t update_game_state(GameData_t *state) {
  Info_t info;

  info.score = state->score;
  info.level = state->level;
  info.speed = state->speed;

  info.field = create_matrix(FIELD_H, FIELD_W);

  for (int i = 0; i < FIELD_H; i++) {
    for (int j = 0; j < FIELD_W; j++) {
      info.field[i][j] = state->field[i][j];
    }
  }

  new_field(state, info.field);

  int size = state->next_block_size;
  info.next = create_matrix(size, size);
  copy_matrix(info.next, state->next_block, size, size);
  info.next_size = size;
  info.next_color = state->next_block_color;

  info.high_score = state->mvp_score;
  info.pause = state->is_paused_flag;

  return info;
}

void new_field(GameData_t *state, int **field) {
  for (int i = 0; i < FIELD_H; i++) {
    for (int j = 0; j < FIELD_W; j++) {
      field[i][j] = state->field[i][j];
    }
  }

  if (state->status == GAME_OVER) return;

  for (int i = 0; i < state->block_size; i++) {
    for (int j = 0; j < state->block_size; j++) {
      int x = state->coord_x + j;
      int y = state->coord_y + i;

      if (state->block[i][j] == 1 && y >= 0 && y < FIELD_H && x >= 0 &&
          x < FIELD_W && field[y][x] == 0) {
        field[y][x] = state->block_color;
      }
    }
  }
}

int **create_matrix(int rows, int colums) {
  int **matrix = (int **)calloc(rows, sizeof(int *));
  if (matrix) {
    for (int i = 0; i < rows; i++) {
      matrix[i] = (int *)calloc(colums, sizeof(int));
      if (!matrix[i]) free_matrix(i, matrix);
    }
  }

  return matrix;
}

int **gen_block(int *size_block, int *block_type) {
  *block_type = rand() % 7;
  *size_block = (*block_type == 0) ? 4 : (*block_type == 3) ? 2 : 3;

  int **block_mtx = create_matrix(*size_block, *size_block);

  switch (*block_type) {
    case 0:
      block_mtx[0][0] = block_mtx[0][1] = block_mtx[0][2] = block_mtx[0][3] = 1;
      break;
    case 1:
      block_mtx[0][0] = block_mtx[1][0] = block_mtx[1][1] = block_mtx[1][2] = 1;
      break;
    case 2:
      block_mtx[1][0] = block_mtx[1][1] = block_mtx[1][2] = block_mtx[0][2] = 1;
      break;
    case 3:
      block_mtx[0][0] = block_mtx[0][1] = block_mtx[1][0] = block_mtx[1][1] = 1;
      break;
    case 4:
      block_mtx[1][0] = block_mtx[1][1] = block_mtx[0][1] = block_mtx[0][2] = 1;
      break;
    case 5:
      block_mtx[0][1] = block_mtx[1][0] = block_mtx[1][1] = block_mtx[1][2] = 1;
      break;
    case 6:
      block_mtx[0][0] = block_mtx[0][1] = block_mtx[1][1] = block_mtx[1][2] = 1;
      break;
  }

  return block_mtx;
}

void rotation(GameData_t *state) {
  int can_move = 1;

  int **temp_figure = create_matrix(state->block_size, state->block_size);

  for (int i = 0; i < state->block_size; i++) {
    for (int j = 0; j < state->block_size; j++) {
      temp_figure[j][state->block_size - i - 1] = state->block[i][j];
    }
  }

  for (int i = 0; i < state->block_size; i++) {
    for (int j = 0; j < state->block_size; j++) {
      int x = state->coord_x + j;
      int y = state->coord_y + i;

      if (temp_figure[i][j] == 1 && (y >= FIELD_H || y < 0 || x >= FIELD_W ||
                                     x < 0 || state->field[y][x] == 1))
        can_move = 0;
    }
  }

  if (can_move) {
    free_matrix(state->block_size, state->block);
    state->block = temp_figure;
  } else {
    free_matrix(state->block_size, temp_figure);
  }
}

void copy_matrix(int **dest, int **src, int H, int W) {
  for (int i = 0; i < H; i++) {
    for (int j = 0; j < W; j++) {
      dest[i][j] = src[i][j];
    }
  }
}

long long current_time() {
  struct timeval t;
  gettimeofday(&t, NULL);

  return (long long)t.tv_sec * 1000 + t.tv_usec / 1000;
}

void free_matrix(int H, int **matrix) {
  for (int i = 0; i < H; i++) {
    free(matrix[i]);
  }

  free(matrix);
}

void free_old_info_and_field(Info_t *info) {
  free_matrix(FIELD_H, info->field);
  free_matrix(info->next_size, info->next);
}

Actions_t get_actions(int ch) {
  Actions_t actions = {-1};
  switch (ch) {
    case 's':
      actions = Start;
      break;
    case TERMINATE_BUTTON:
      actions = Terminate;
      break;
    case 'p':
      actions = Pause;
      break;
    case KEY_LEFT:
      actions = Left;
      break;
    case KEY_RIGHT:
      actions = Right;
      break;
    case KEY_DOWN:
      actions = Down;
      break;
    case KEY_UP:
      actions = Up;
      break;
  }
  return actions;
}

void action_buttoms(GameData_t *state, Actions_t actions) {
  switch (state->status) {
    case GAME_INIT:
      if (actions == Terminate) {
        finish_game(state);
      } else if (actions == Start) {
        state->status = GAME_SPAWN;
      }
      break;

    case GAME_SPAWN:
      if (actions == Terminate || state->victory_flag) {
        finish_game(state);
      } else {
        spawn_figure(state);
      }
      break;

    case GAME_MOVING:
      move_figure(state, actions);
      break;
    case GAME_SHIFTING:
      move_down(state);
      break;
    case GAME_ATTACHING:
      attach_figure(state);
      break;
    case GAME_OVER:
      finish_game(state);
      break;
  }
}

void move_figure(GameData_t *state, Actions_t actions) {
  if (!state->is_paused_flag || actions == Pause || actions == Terminate) {
    switch (actions) {
      case Left:
        move_left(state);
        break;
      case Right:
        move_right(state);
        break;
      case Down:
        move_down(state);
        break;
      case Up:
        rotation(state);
        break;
      case Pause:
        state->is_paused_flag = !state->is_paused_flag;
        break;
      case Terminate:
        finish_game(state);
        break;
      case Start:
        if (state->status == GAME_INIT) state->status = GAME_SPAWN;
        break;
    }
  }

  if (timer(state, state->speed) && !state->is_paused_flag) {
    state->status = GAME_SHIFTING;
  }
}

void spawn_figure(GameData_t *state) {
  free_matrix(state->block_size, state->block);
  state->block = state->next_block;
  state->block_size = state->next_block_size;
  state->block_color = state->next_block_color;

  int size, type;
  state->next_block = gen_block(&size, &type);
  state->next_block_size = size;
  state->next_block_color = type + 7;
  state->coord_x = (state->block_size == 2) ? 4 : 3;
  state->coord_y = 0;

  state->time = current_time();
  state->status = GAME_MOVING;
}

int timer(GameData_t *state, int delay) {
  int result = 0;
  long long time = current_time();

  if (time - state->time >= delay) {
    state->time = time;
    result = 1;
  }
  return result;
}

int figure_is_attaching(GameData_t *state) {
  for (int i = 0; i < state->block_size; i++) {
    for (int j = 0; j < state->block_size; j++) {
      if (state->block[i][j] == 1) {
        int y = state->coord_y + i + 1;
        int x = state->coord_x + j;
        if (y >= FIELD_H || (y >= 0 && state->field[y][x] != 0)) {
          return 1;
        }
      }
    }
  }
  return 0;
}

void move_left(GameData_t *state) {
  int can_move = 1;

  for (int i = 0; i < state->block_size; i++) {
    for (int j = 0; j < state->block_size; j++) {
      int x = state->coord_x + j - 1;
      int y = state->coord_y + i;

      if (state->block[i][j] == 1 &&
          (x >= FIELD_W || x < 0 || state->field[y][x] != 0)) {
        can_move = 0;
      }
    }
  }

  if (can_move) {
    state->coord_x--;
  }

  state->status = figure_is_attaching(state) ? GAME_ATTACHING : GAME_MOVING;
}

void move_right(GameData_t *state) {
  int can_move = 1;

  for (int i = 0; i < state->block_size; i++) {
    for (int j = 0; j < state->block_size; j++) {
      int x = state->coord_x + j + 1;
      int y = state->coord_y + i;

      if (state->block[i][j] == 1 &&
          (x >= FIELD_W || x < 0 || state->field[y][x] != 0)) {
        can_move = false;
      }
    }
  }

  if (can_move) {
    state->coord_x++;
  }

  state->status = figure_is_attaching(state) ? GAME_ATTACHING : GAME_MOVING;
}

void move_down(GameData_t *state) {
  if (!figure_is_attaching(state)) {
    state->coord_y++;
    state->status = GAME_MOVING;
  } else {
    state->status = GAME_ATTACHING;
  }
}

void attach_figure(GameData_t *state) {
  for (int i = 0; i < state->block_size; i++) {
    for (int j = 0; j < state->block_size; j++) {
      if (state->block[i][j] == 1) {
        int y = state->coord_y + i;
        int x = state->coord_x + j;

        if (y >= 0 && y < FIELD_H && x >= 0 && x < FIELD_W) {
          if (state->field[y][x] == 0) {
            state->field[y][x] = state->block_color;
          }
        }
      }
    }
  }
  if (check_top_line(state)) {
    state->status = GAME_OVER;
  } else {
    process_full_lines(state);
    state->status = GAME_SPAWN;
  }
}

int check_top_line(GameData_t *state) {
  for (int j = 0; j < FIELD_W; j++) {
    if (state->field[0][j] != 0) {
      return 1;
    }
  }
  return 0;
}

void process_full_lines(GameData_t *state) {
  int num_full_lines = 0;

  for (int i = FIELD_H - 1; i >= 0; i--) {
    bool is_full = true;

    for (int j = 0; j < FIELD_W && is_full; j++) {
      if (state->field[i][j] == 0) {
        is_full = false;
      }
    }

    if (is_full) {
      shift_lines(state, i);
      fill_top_line(state);
      num_full_lines++;
      i++;
    }
  }

  new_score_and_level(state, num_full_lines);
}

void shift_lines(GameData_t *state, int i) {
  for (int j = i; j > 0; j--) {
    for (int k = 0; k < FIELD_W; k++) {
      state->field[j][k] = state->field[j - 1][k];
    }
  }
}

void fill_top_line(GameData_t *state) {
  for (int j = 0; j < FIELD_W; j++) {
    state->field[0][j] = 0;
  }
}

void new_score_and_level(GameData_t *state, int count_flines) {
  switch (count_flines) {
    case 1:
      state->score += 100;
      break;
    case 2:
      state->score += 300;
      break;
    case 3:
      state->score += 700;
      break;
    case 4:
      state->score += 1500;
      break;
  }

  if (state->score >= NEW_LEVEL_TRESHOLD) {
    state->level = state->score / NEW_LEVEL_TRESHOLD + 1;
    state->speed = SPEED + SPEED_STEP;
  }

  if (state->level > MAX_LEVEL) {
    state->victory_flag = 1;
  }
  if (state->score > state->mvp_score) {
    FILE *file = fopen("mvp.txt", "w");

    if (file) {
      fprintf(file, "%d", state->score);
      fclose(file);
    }
  }
}

void finish_game(GameData_t *state) {
  if (state->status != GAME_OVER && !state->victory_flag) {
    state->game_on_flag = 0;
  }
  free_matrix(FIELD_H, state->field);
  free_matrix(state->block_size, state->block);
  free_matrix(state->next_block_size, state->next_block);
}
