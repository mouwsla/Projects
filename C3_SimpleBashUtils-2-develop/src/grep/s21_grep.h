#ifndef S21_GREP_H
#define S21_GREP_H

#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct options {
  char *pattern;
  int i;
  int v;
  int c;
  int l;
  int n;
  char **files;
  int file_count;
} opt;

int print_usage();
int parse_arguments(int argc, char **argv, opt *options);
int process_file(char *filename, opt *options);

#endif