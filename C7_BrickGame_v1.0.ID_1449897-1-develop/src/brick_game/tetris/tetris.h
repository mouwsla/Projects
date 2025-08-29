#ifndef TETRIS_H
#define TETRIS_H

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "../../game.h"

#define SPEED 500
#define SPEED_STEP 150
#define NEW_LEVEL_TRESHOLD 600
#define MAX_LEVEL 10

typedef enum { Start, Pause, Terminate, Left, Right, Up, Down } Actions_t;

typedef enum {
  GAME_INIT,
  GAME_SPAWN,
  GAME_MOVING,
  GAME_SHIFTING,
  GAME_ATTACHING,
  GAME_OVER
} GamePhase_t;

typedef struct {
  int status;
  int **field;
  int **block;
  int block_size;
  int **next_block;
  int next_block_size;
  int coord_x;
  int coord_y;
  int score;
  int mvp_score;
  int level;
  int speed;
  int is_paused_flag;
  int game_on_flag;
  int victory_flag;
  long long time;
  int block_color;
  int next_block_color;
} GameData_t;

int **create_matrix(int rows, int colums);
int **gen_block(int *size_block, int *block_type);
void rotation(GameData_t *state);
void free_matrix(int H, int **matrix);
void copy_matrix(int **dest, int **src, int H, int W);
void game_start();
Info_t update_game_state(GameData_t *state);
void new_field(GameData_t *state, int **field);
long long current_time();
void free_old_info_and_field(Info_t *info);
Actions_t get_actions(int ch);
void action_buttoms(GameData_t *state, Actions_t actions);
void move_figure(GameData_t *state, Actions_t actions);
void spawn_figure(GameData_t *state);
int timer(GameData_t *state, int delay);
int figure_is_attaching(GameData_t *state);
void move_left(GameData_t *state);
void move_right(GameData_t *state);
void move_down(GameData_t *state);
void attach_figure(GameData_t *state);
int check_top_line(GameData_t *gs);
void process_full_lines(GameData_t *state);
void shift_lines(GameData_t *state, int i);
void fill_top_line(GameData_t *state);
void new_score_and_level(GameData_t *state, int count_flines);
void finish_game(GameData_t *state);
GameData_t *get_game_state();

#endif