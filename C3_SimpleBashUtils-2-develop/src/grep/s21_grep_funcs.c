#include "s21_grep.h"

int print_usage() {
  fprintf(stderr,
          "usage: s21grep "
          "[-abcdDEFGHhIiJLlMmnOopqRSsUVvwXxZz] [-A num] [-B num] "
          "[-C[num]]\n\t[-e pattern] [-f file] [--binary-files=value] "
          "[--color=when]\n\t[--context[=num]] [--directories=action] "
          "[--label] [--line-buffered]\n\t[--null] [pattern] [file ...]\n");
  return 1;
}

int parse_arguments(int argc, char **argv, opt *options) {
  int pattern_set = 0;
  options->file_count = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-e") == 0) {
      if (i + 1 < argc) {
        options->pattern = argv[++i];
        pattern_set = 1;
      } else {
        fprintf(
            stderr,
            "s21grep: option requires an argument -- %c\nusage: s21grep "
            "[-abcdDEFGHhIiJLlMmnOopqRSsUVvwXxZz] [-A num] [-B num] "
            "[-C[num]]\n\t[-e pattern] [-f file] [--binary-files=value] "
            "[--color=when]\n\t[--context[=num]] [--directories=action] "
            "[--label] [--line-buffered]\n\t[--null] [pattern] [file ...]\n",
            argv[i][1]);
        return 1;
      }
    } else if (strcmp(argv[i], "-i") == 0) {
      options->i = 1;
    } else if (strcmp(argv[i], "-v") == 0) {
      options->v = 1;
    } else if (strcmp(argv[i], "-c") == 0) {
      options->c = 1;
    } else if (strcmp(argv[i], "-l") == 0) {
      options->l = 1;
    } else if (strcmp(argv[i], "-n") == 0) {
      options->n = 1;
    } else if (argv[i][0] == '-') {
      fprintf(stderr, "error: Unknown flag %s\n", argv[i]);
      return 1;
    } else if (!pattern_set) {
      options->pattern = argv[i];
      pattern_set = 1;
    } else {
      options->files = &argv[i];
      options->file_count = argc - i;
      break;
    }
  }

  return 0;
}

int process_file(char *filename, opt *options) {
  FILE *file = fopen(filename, "rt");
  if (!file) {
    fprintf(stderr, "s21grep: %s: No such file or directory\n", filename);
    return 1;
  }

  regex_t regex;
  int regex_flags = REG_EXTENDED;
  if (options->i) regex_flags |= REG_ICASE;

  if (regcomp(&regex, options->pattern, regex_flags) != 0) {
    fprintf(stderr, "error: Invalid regex pattern: %s\n", options->pattern);
    fclose(file);
    return 1;
  }

  char line[4096];
  int match_count = 0;
  int line_num = 0;

  while (fgets(line, sizeof(line), file)) {
    line_num++;
    line[strcspn(line, "\n")] = '\0';

    int match = regexec(&regex, line, 0, NULL, 0) == 0;
    if (options->v) {
      match = !match;
    }

    if (match) {
      match_count++;
      if (options->l) {
        printf("%s\n", filename);
        break;
      }
      if (!options->c) {
        if (options->n && options->file_count == 1) {
          printf("%d:%s\n", line_num, line);
        } else if (options->n && options->file_count > 1) {
          printf("%s:%d:%s\n", filename, line_num, line);
        } else {
          printf("%s\n", line);
        }
      }
    }
  }

  if (options->c && options->file_count == 1) {
    printf("%d\n", match_count);
  } else if (options->c && options->file_count > 1) {
    printf("%s:%d\n", filename, match_count);
  }

  regfree(&regex);
  fclose(file);
  if (match_count > 0) {
    return 1;
  } else {
    return 0;
  }
}
