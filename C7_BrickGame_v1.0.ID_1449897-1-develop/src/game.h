#ifndef GAME_H
#define GAME_H

#define FIELD_H 20
#define FIELD_W 10
#define TERMINATE_BUTTON 27

typedef struct {
  int **field;
  int **next;
  int next_size;
  int next_color;
  int score;
  int high_score;
  int level;
  int speed;
  int pause;
} Info_t;

#include "brick_game/tetris/tetris.h"
#include "gui/cli/interface.h"

#endif