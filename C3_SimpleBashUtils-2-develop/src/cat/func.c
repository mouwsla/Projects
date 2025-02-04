#include "s21_cat.h"

void show_nonprinting(char *buffer) {
  unsigned char ch = *buffer;
  if (ch > 127 && ch < 160) {
    printf("M-^");
  }
  if ((ch < 32 && ch != '\n' && ch != '\t') || ch == 127) {
    printf("^");
  }
  if ((ch < 32 || (ch > 126 && ch < 160)) && ch != '\n' && ch != '\t') {
    if (ch > 126) {
      ch = ch - 128 + 64;
    } else {
      ch += 64;
    }
  }
  *buffer = ch;
}

void format_output(char *buffer, int bytes_read, int *count, int *empty_line,
                   opt options) {
  int b_flag = 1, n_check = 0;
  for (int i = 0; i < bytes_read; i++) {
    int s_check = 1, t_check = 1;
    char prev = '\n';
    if (i != 0) {
      prev = buffer[i - 1];
    }
    if (buffer[i] == '\n') {
      *empty_line += 1;
    } else {
      *empty_line = 0;
    }
    if (options.s && *empty_line > 2) {
      s_check = 0;
    }
    if ((options.b && (b_flag || prev == '\n') && buffer[i] != '\n') ||
        (options.n && !options.b && prev == '\n')) {
      if (s_check || (i != bytes_read - 1 && buffer[i + 1] != '\n')) {
        printf("%6d\t", *count);
        *count += 1;
      } else {
        n_check = 1;
      }
    }
    b_flag = 0;
    if (options.e && buffer[i] == '\n' && s_check) {
      printf("$");
    }
    if (options.t && buffer[i] == '\t') {
      printf("^I");
      t_check = 0;
    }
    if (s_check && t_check) {
      if (n_check) {
        printf("%6d\t", *count);
        *count += 1;
        n_check = 0;
      }
      if (options.v && buffer[i] != '\t' && buffer[i] != '\n') {
        show_nonprinting(&buffer[i]);
      }
      printf("%c", buffer[i]);
    } else if (!s_check && t_check) {
      buffer[i] = '\0';
      b_flag = 1;
    }
  }
}

void read_text(int fd, opt options) {
  char buffer[4096];
  int bytes_read;

  int count = 1, empty_line = 1;

  while ((bytes_read = read(fd, buffer, 4096)) > 0) {
    if (options.b || options.n || options.e || options.s || options.t ||
        options.v) {
      format_output(buffer, bytes_read, &count, &empty_line, options);
    } else {
      printf("%.*s", bytes_read, buffer);
    }
  }
}

int parser(int argc, char **argv, opt *options) {
  struct option long_options[] = {{"number-nonblank", 0, 0, 'b'},
                                  {"squeeze-blank", 0, 0, 's'},
                                  {"number", 0, 0, 'n'},
                                  {0, 0, 0, 0}};
  int opt;
  int option_index = 0;
  int flag = 1;
  while ((opt = getopt_long(argc, argv, "+benstvET", long_options,
                            &option_index)) != -1 &&
         flag) {
    switch (opt) {
      case 'b':
        options->b = 1;
        break;
      case 'e':
        options->e = 1;
        options->v = 1;
        break;
      case 'n':
        options->n = 1;
        break;
      case 's':
        options->s = 1;
        break;
      case 't':
        options->t = 1;
        options->v = 1;
        break;
      case 'v':
        options->v = 1;
        break;
      case 'E':
        options->e = 1;
        break;
      case 'T':
        options->t = 1;
        break;
      default:
        flag = 0;
        fprintf(stderr, "usage: s21cat [-benstvET] [file...]\n");
    }
  }
  return flag;
}