#include "s21_string.h"

char *s21_strtok(char *str, const char *delim) {
  static char *nextToken = S21_NULL;
  char *currentToken;

  if (str != S21_NULL) {
    nextToken = str;
  }
  if (nextToken == S21_NULL) {
    return S21_NULL;
  }
  currentToken = nextToken;
  while (*currentToken && s21_strchr(delim, *currentToken)) {
    currentToken++;
  }

  if (*currentToken == '\0') {
    nextToken = S21_NULL;
    return S21_NULL;
  }
  nextToken = currentToken;
  while (*nextToken && !s21_strchr(delim, *nextToken)) {
    nextToken++;
  }
  if (*nextToken == '\0') {
    nextToken = S21_NULL;
  } else {
    *nextToken = '\0';
    nextToken++;
  }
  return currentToken;
}