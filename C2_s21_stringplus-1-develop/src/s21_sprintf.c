#include "s21_string.h"

void handle_char(char **ptr, char c, Format fmt) {
  if (!fmt.minus && fmt.width > 1) {
    for (int i = 1; i < fmt.width; i++) {
      **ptr = ' ';
      (*ptr)++;
    }
  }

  **ptr = c;
  (*ptr)++;

  if (fmt.minus && fmt.width > 1) {
    for (int i = 1; i < fmt.width; i++) {
      **ptr = ' ';
      (*ptr)++;
    }
  }
}

void handle_signed_int(char **ptr, int value, Format fmt) {
  char buffer[1024];
  int index = 0;

  if (value < 0) {
    buffer[index++] = '-';
    value = -value;
  } else if (fmt.plus) {
    buffer[index++] = '+';
  } else if (fmt.space) {
    buffer[index++] = ' ';
  }

  char temp[1024];
  int temp_index = 0;
  do {
    temp[temp_index++] = (value % 10) + '0';
    value /= 10;
  } while (value > 0);

  while (temp_index > 0) {
    buffer[index++] = temp[--temp_index];
  }
  buffer[index] = '\0';

  int length = s21_strlen(buffer);
  int padding = (fmt.width > length) ? fmt.width - length : 0;

  if (!fmt.minus) {
    for (int i = 0; i < padding; i++) {
      **ptr = ' ';
      (*ptr)++;
    }
  }

  s21_strcpy(*ptr, buffer);
  *ptr += length;

  if (fmt.minus) {
    for (int i = 0; i < padding; i++) {
      **ptr = ' ';
      (*ptr)++;
    }
  }
}

void handle_float(char **ptr, double value, Format fmt) {
  char buffer[1024];
  int length = 0;

  if (value < 0) {
    buffer[length++] = '-';
    value = -value;
  } else if (fmt.plus) {
    buffer[length++] = '+';
  } else if (fmt.space) {
    buffer[length++] = ' ';
  }

  int int_part = (int)value;
  double frac_part = value - int_part;

  length += s21_sprintf(buffer + length, "%d", int_part);

  if (fmt.precision < 0) fmt.precision = 6;
  frac_part += pow(10, -fmt.precision) / 2;

  buffer[length++] = '.';
  for (int i = 0; i < fmt.precision; i++) {
    frac_part *= 10;
    int digit = (int)frac_part;
    buffer[length++] = digit + '0';
    frac_part -= digit;
  }
  buffer[length] = '\0';

  int padding = (fmt.width > length) ? fmt.width - length : 0;
  if (!fmt.minus) {
    for (int i = 0; i < padding; i++) {
      **ptr = ' ';
      (*ptr)++;
    }
  }

  s21_strcpy(*ptr, buffer);
  *ptr += length;

  if (fmt.minus) {
    for (int i = 0; i < padding; i++) {
      **ptr = ' ';
      (*ptr)++;
    }
  }
}

void handle_string(char **ptr, char *value, Format fmt) {
  int length = s21_strlen(value);
  int display_length =
      (fmt.precision >= 0 && fmt.precision < length) ? fmt.precision : length;
  int padding = (fmt.width > display_length) ? fmt.width - display_length : 0;

  if (!fmt.minus) {
    for (int i = 0; i < padding; i++) {
      **ptr = ' ';
      (*ptr)++;
    }
  }

  s21_strncpy(*ptr, value, display_length);
  *ptr += display_length;

  if (fmt.minus) {
    for (int i = 0; i < padding; i++) {
      **ptr = ' ';
      (*ptr)++;
    }
  }
}

void handle_unsigned_int(char **ptr, unsigned int value, Format fmt) {
  char buffer[1024];
  int index = 0;

  char temp[1024];
  int temp_index = 0;
  do {
    temp[temp_index++] = (value % 10) + '0';
    value /= 10;
  } while (value > 0);

  while (temp_index > 0) {
    buffer[index++] = temp[--temp_index];
  }

  buffer[index] = '\0';

  int length = s21_strlen(buffer);
  int padding = (fmt.width > length) ? fmt.width - length : 0;

  if (!fmt.minus) {
    for (int i = 0; i < padding; i++) {
      **ptr = ' ';
      (*ptr)++;
    }
  }

  s21_strcpy(*ptr, buffer);
  *ptr += length;

  if (fmt.minus) {
    for (int i = 0; i < padding; i++) {
      **ptr = ' ';
      (*ptr)++;
    }
  }
}

void handle_percent(char **ptr) {
  **ptr = '%';
  (*ptr)++;
}

void parser_format(const char **fmt, Format *sing) {
  sing->plus = 0;
  sing->minus = 0;
  sing->precision = -1;
  sing->width = 0;
  sing->length = '\0';
  sing->space = 0;

  while (**fmt == '+' || **fmt == '-' || **fmt == ' ') {
    if (**fmt == '+') sing->plus = 1;
    if (**fmt == '-') sing->minus = 1;
    if (**fmt == ' ') sing->space = 1;
    (*fmt)++;
  }

  while (**fmt >= '0' && **fmt <= '9') {
    sing->width = sing->width * 10 + (**fmt - '0');
    (*fmt)++;
  }

  if (**fmt == '.') {
    (*fmt)++;
    sing->precision = 0;
    while (**fmt >= '0' && **fmt <= '9') {
      sing->precision = sing->precision * 10 + (**fmt - '0');
      (*fmt)++;
    }
  }

  if (**fmt == 'h' || **fmt == 'l') {
    sing->length = **fmt;
    (*fmt)++;
  }
}

int s21_sprintf(char *str, const char *format, ...) {
  va_list args;
  va_start(args, format);

  char *ptr = str;
  const char *fmt = format;

  while (*fmt) {
    if (*fmt == '%') {
      fmt++;
      Format sing;
      parser_format(&fmt, &sing);

      switch (*fmt) {
        case 'c':
          handle_char(&ptr, (char)va_arg(args, int), sing);
          break;
        case 'd':
        case 'i':
          handle_signed_int(&ptr, va_arg(args, int), sing);
          break;
        case 'f':
          handle_float(&ptr, va_arg(args, double), sing);
          break;
        case 's':
          handle_string(&ptr, va_arg(args, char *), sing);
          break;
        case 'u':
          handle_unsigned_int(&ptr, va_arg(args, unsigned int), sing);
          break;
        case '%':
          handle_percent(&ptr);
          break;
      }
    } else {
      *ptr++ = *fmt;
    }
    fmt++;
  }

  *ptr = '\0';
  va_end(args);
  return ptr - str;
}
