// SPDX-License-Identifier: MIT
#include "test_framework.h"

// Declare test suite functions
void run_data_structures_tests (void);
void run_file_operations_tests (void);
void run_gap_buffer_tests (void);
void run_undo_tests (void);

int
main ()
{
  init_test_framework ();

  run_gap_buffer_tests ();
  run_data_structures_tests ();
  run_file_operations_tests ();
  run_undo_tests ();

  print_test_summary ();

  return tests_failed > 0 ? 1 : 0;
}
