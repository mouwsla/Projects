#ifndef S21_CAT_H
#define S21_CAT_H

#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct options {
  int b;
  int e;
  int n;
  int s;
  int t;
  int v;
} opt;

void show_nonprinting(char *buffer);
void format_output(char *buffer, int bytes_read, int *count, int *empty_line,
                   opt options);
void read_text(int fd, opt options);
int parser(int argc, char **argv, opt *options);

#endif