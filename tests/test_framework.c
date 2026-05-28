// SPDX-License-Identifier: MIT
#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>

// Global variables - defined here
int tests_run = 0;
int tests_passed = 0;
int tests_failed = 0;

const char *current_test_case_name = NULL;

void
init_test_framework (void)
{
  printf (TEST_COLOR_BLUE "Initializing Test Framework...\n" TEST_COLOR_RESET);
  tests_run = 0;
  tests_passed = 0;
  tests_failed = 0;
}

void
print_test_summary (void)
{
  printf (TEST_COLOR_BLUE "\n=== FINAL TEST SUMMARY ===\n" TEST_COLOR_RESET);
  printf (TEST_COLOR_GREEN "Total passed: %d\n" TEST_COLOR_RESET,
          tests_passed);

  if (tests_failed > 0)
    {
      printf (TEST_COLOR_RED "Total failed: %d\n" TEST_COLOR_RESET,
              tests_failed);
      printf (TEST_COLOR_RED "SUCCESS RATE: %.1f%%\n" TEST_COLOR_RESET,
              (float)tests_passed / (tests_passed + tests_failed) * 100.0f);
    }
  else
    {
      printf (TEST_COLOR_GREEN "SUCCESS RATE: 100.0%%\n" TEST_COLOR_RESET);
      printf (TEST_COLOR_GREEN "✓ All tests passed!\n" TEST_COLOR_RESET);
    }
}
