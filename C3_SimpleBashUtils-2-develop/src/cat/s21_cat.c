#include "s21_cat.h"

int main(int argc, char **argv) {
  opt options = {0};

  if (argc == 1) {
    read_text(STDIN_FILENO, options);
  } else if (parser(argc, argv, &options)) {
    if (optind == argc) {
      read_text(STDIN_FILENO, options);
    }

    for (int i = optind; i < argc; i++) {
      int fd = open(argv[i], O_RDONLY);
      if (fd != -1) {
        read_text(fd, options);
      } else {
        fprintf(stderr, "s21cat: %s: No such file or directory\n", argv[i]);
      }
    }
  }

  return 0;
}