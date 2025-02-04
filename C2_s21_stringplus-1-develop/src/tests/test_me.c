#include "test_me.h"

int main(void) {
  int failed = 0;
  Suite *s21_string_test[] = {

      test_memchr(),
      test_memcmp(),
      test_memcpy(),
      test_memset(),
      test_strncat(),
      test_strchr(),
      test_strncmp(),
      test_strncpy(),
      test_strcspn(),
      test_strerror(),
      test_strlen(),
      test_strpbrk(),
      test_strrchr(),
      test_strstr(),
      test_strtok(),
      test_strcpy(),

      test_sprintf_c(),
      test_sprintf_signed_i(),
      test_sprintf_f(),
      test_sprintf_unsigned(),
      test_sprintf_string(),
      test_sprintf_d(),

      test_to_upper(),
      test_to_lower(),
      test_insert(),
      test_trim(),

      NULL};

  for (int i = 0; s21_string_test[i] != NULL; i++) {
    SRunner *sr = srunner_create(s21_string_test[i]);
    srunner_run_all(sr, CK_NORMAL);
    failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    if (failed == 0)
      printf("\x1B[32mSuccess\n\x1B[0m");
    else
      printf("\x1B[31mFailed\n\x1B[0m");

    failed = 0;
  }

  return 0;
}
