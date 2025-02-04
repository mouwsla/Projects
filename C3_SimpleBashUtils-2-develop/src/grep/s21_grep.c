#include "s21_grep.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage();
  }

  opt options = {0};

  if (parse_arguments(argc, argv, &options) != 0) {
    return 1;
  }

  int files_with_matches = 0;

  for (int i = 0; i < options.file_count; i++) {
    int result = process_file(options.files[i], &options);
    if (result > 0) {
      files_with_matches++;
    }
  }
  if (options.l && files_with_matches == 0) {
    return 1;
  }

  return 0;
}
