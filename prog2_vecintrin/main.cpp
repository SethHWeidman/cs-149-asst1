#include "CS149intrin.h"
#include "logger.h"
#include <algorithm>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
using namespace std;

#define EXP_MAX 10

Logger CS149Logger;

void usage(const char *progname);
void initValue(float *values, int *exponents, float *output, float *gold, unsigned int N);
void absSerial(float *values, float *output, int N);
void absVector(float *values, float *output, int N);
void clampedExpSerial(float *values, int *exponents, float *output, int N);
void clampedExpVector(float *values, int *exponents, float *output, int N);
float arraySumSerial(float *values, int N);
float arraySumVector(float *values, int N);
bool verifyResult(float *values, int *exponents, float *output, float *gold, int N);

int main(int argc, char *argv[]) {
  int N = 16;
  bool printLog = false;

  // parse commandline options ////////////////////////////////////////////
  int opt;
  static struct option long_options[] = {
      {"size", 1, 0, 's'}, {"log", 0, 0, 'l'}, {"help", 0, 0, '?'}, {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "s:l?", long_options, NULL)) != EOF) {

    switch (opt) {
    case 's':
      N = atoi(optarg);
      if (N <= 0) {
        printf("Error: Workload size is set to %d (<0).\n", N);
        return -1;
      }
      break;
    case 'l':
      printLog = true;
      break;
    case '?':
    default:
      usage(argv[0]);
      return 1;
    }
  }

  float *values = new float[N + VECTOR_WIDTH];
  int *exponents = new int[N + VECTOR_WIDTH];
  float *output = new float[N + VECTOR_WIDTH];
  float *gold = new float[N + VECTOR_WIDTH];
  initValue(values, exponents, output, gold, N);

  clampedExpSerial(values, exponents, gold, N);
  clampedExpVector(values, exponents, output, N);

  // absSerial(values, gold, N);
  // absVector(values, output, N);

  printf("\e[1;31mCLAMPED EXPONENT\e[0m (required) \n");
  bool clampedCorrect = verifyResult(values, exponents, output, gold, N);
  if (printLog)
    CS149Logger.printLog();
  CS149Logger.printStats();

  printf("************************ Result Verification *************************\n");
  if (!clampedCorrect) {
    printf("@@@ Failed!!!\n");
  } else {
    printf("Passed!!!\n");
  }

  // printf("\n\e[1;31mARRAY SUM\e[0m (bonus) \n");
  // if (N % VECTOR_WIDTH == 0) {
  //   float sumGold = arraySumSerial(values, N);
  //   float sumOutput = arraySumVector(values, N);
  //   float epsilon = 0.1;
  //   bool sumCorrect = abs(sumGold - sumOutput) < epsilon * 2;
  //   if (!sumCorrect) {
  //     printf("Expected %f, got %f\n.", sumGold, sumOutput);
  //     printf("@@@ Failed!!!\n");
  //   } else {
  //     printf("Passed!!!\n");
  //   }
  // } else {
  //   printf("Must have N %% VECTOR_WIDTH == 0 for this problem (VECTOR_WIDTH is %d)\n",
  //          VECTOR_WIDTH);
  // }

  delete[] values;
  delete[] exponents;
  delete[] output;
  delete[] gold;

  return 0;
}

void usage(const char *progname) {
  printf("Usage: %s [options]\n", progname);
  printf("Program Options:\n");
  printf("  -s  --size <N>     Use workload size N (Default = 16)\n");
  printf("  -l  --log          Print vector unit execution log\n");
  printf("  -?  --help         This message\n");
}

void initValue(float *values, int *exponents, float *output, float *gold, unsigned int N) {

  for (unsigned int i = 0; i < N + VECTOR_WIDTH; i++) {
    // random input values
    values[i] = -1.f + 4.f * static_cast<float>(rand()) / RAND_MAX;
    exponents[i] = rand() % EXP_MAX;
    output[i] = 0.f;
    gold[i] = 0.f;
  }
}

bool verifyResult(float *values, int *exponents, float *output, float *gold, int N) {
  int incorrect = -1;
  float epsilon = 0.00001;
  for (int i = 0; i < N + VECTOR_WIDTH; i++) {
    if (abs(output[i] - gold[i]) > epsilon) {
      incorrect = i;
      break;
    }
  }

  if (incorrect != -1) {
    if (incorrect >= N)
      printf("You have written to out of bound value!\n");
    printf("Wrong calculation at value[%d]!\n", incorrect);
    printf("value  = ");
    for (int i = 0; i < N; i++) {
      printf("% f ", values[i]);
    }
    printf("\n");

    printf("exp    = ");
    for (int i = 0; i < N; i++) {
      printf("% 9d ", exponents[i]);
    }
    printf("\n");

    printf("output = ");
    for (int i = 0; i < N; i++) {
      printf("% f ", output[i]);
    }
    printf("\n");

    printf("gold   = ");
    for (int i = 0; i < N; i++) {
      printf("% f ", gold[i]);
    }
    printf("\n");
    return false;
  }
  printf("Results matched with answer!\n");
  return true;
}

// computes the absolute value of all elements in the input array
// values, stores result in output
void absSerial(float *values, float *output, int N) {
  for (int i = 0; i < N; i++) {
    float x = values[i];
    if (x < 0) {
      output[i] = -x;
    } else {
      output[i] = x;
    }
  }
}

// implementation of absSerial() above, but it is vectorized using CS149 intrinsics
void absVector(float *values, float *output, int N) {
  __cs149_vec_float x;
  __cs149_vec_float result;
  __cs149_vec_float zero = _cs149_vset_float(0.f);
  __cs149_mask maskAll, maskIsNegative, maskIsNotNegative;

  //  Note: Take a careful look at this loop indexing.  This example
  //  code is not guaranteed to work when (N % VECTOR_WIDTH) != 0.
  //  Why is that the case?
  //
  //  The loop causes an out-of-bounds read if `N` is not a multiple of `VECTOR_WIDTH`.
  //
  //  For example, if N = 23 and VECTOR_WIDTH = 4:
  //  - The final loop iteration starts at i = 20.
  //  - `_cs149_vload_float` will attempt to load 4 elements, accessing indices 20, 21, 22, and 23.
  //  - Since the array's valid indices are 0-22, accessing values[23] reads past the end of the
  //    array, causing undefined behavior.
  //
  //  A correct implementation must handle the remaining elements, either by using a mask for the
  //  final iteration or by processing the leftovers in a separate scalar loop.
  for (int i = 0; i < N; i += VECTOR_WIDTH) {

    // All ones
    maskAll = _cs149_init_ones();

    // All zeros
    maskIsNegative = _cs149_init_ones(0);

    // Load vector of values from contiguous memory addresses
    _cs149_vload_float(x, values + i, maskAll); // x = values[i];

    // Set mask according to predicate
    _cs149_vlt_float(maskIsNegative, x, zero, maskAll); // if (x < 0) {

    // Execute instruction using mask ("if" clause)
    _cs149_vsub_float(result, zero, x, maskIsNegative); //   output[i] = -x;

    // Inverse maskIsNegative to generate "else" mask
    maskIsNotNegative = _cs149_mask_not(maskIsNegative); // } else {

    // Execute instruction ("else" clause)
    _cs149_vload_float(result, values + i, maskIsNotNegative); //   output[i] = x; }

    // Write results back to memory
    _cs149_vstore_float(output + i, result, maskAll);
  }
}

// accepts an array of values and an array of exponents
//
// For each element, compute values[i]^exponents[i] and clamp value to
// 9.999.  Store result in output.
void clampedExpSerial(float *values, int *exponents, float *output, int N) {
  for (int i = 0; i < N; i++) {
    float x = values[i];
    int y = exponents[i];
    if (y == 0) {
      output[i] = 1.f;
    } else {
      float result = x;
      int count = y - 1;
      while (count > 0) {
        result *= x;
        count--;
      }
      if (result > 9.999999f) {
        result = 9.999999f;
      }
      output[i] = result;
    }
  }
}

void clampedExpVector(float *values, int *exponents, float *output, int N) {
  for (int i = 0; i < N; i += VECTOR_WIDTH) {
    // 1. Create a mask to handle the final iteration if N is not a multiple of VECTOR_WIDTH.
    int num_lanes = (i + VECTOR_WIDTH > N) ? (N - i) : VECTOR_WIDTH;
    __cs149_mask active_mask = _cs149_init_ones(num_lanes);

    // 2. Load a vector of values and exponents.
    __cs149_vec_float v_values;
    __cs149_vec_int v_exponents;
    _cs149_vload_float(v_values, values + i, active_mask);
    _cs149_vload_int(v_exponents, exponents + i, active_mask);

    // 3. Define vector constants.
    __cs149_vec_int v_zero_int = _cs149_vset_int(0);
    __cs149_vec_int v_one_int = _cs149_vset_int(1);
    __cs149_vec_float v_clamp_val = _cs149_vset_float(9.999999f);

    // 4. Start building the result.
    // Stores the calculated power (x^y) for each lane
    __cs149_vec_float v_result;
    // Stores the remaining count of multiplications (y-1) for each lane
    __cs149_vec_int v_count;

    // 5. Identify lanes where exponent is 0.
    // We will simply set these values to 1.0f at the end.
    __cs149_mask mask_y_is_0;
    _cs149_veq_int(mask_y_is_0, v_exponents, v_zero_int, active_mask);

    // 6. For lanes where exponent is not 0, perform exponentiation.
    __cs149_mask mask_y_is_not_0 = _cs149_mask_not(mask_y_is_0);
    // and with `active_mask` to prevent processing inactive lanes
    mask_y_is_not_0 = _cs149_mask_and(mask_y_is_not_0, active_mask);

    // Initialize result and count for these lanes (result = x, count = y - 1)
    _cs149_vmove_float(v_result, v_values, mask_y_is_not_0);
    // y - 1
    _cs149_vsub_int(v_count, v_exponents, v_one_int, mask_y_is_not_0);

    // Perform repeated multiplication (simulating `while(count > 0)`).
    // This loop runs up to the maximum possible exponent.
    for (int j = 0; j < EXP_MAX - 1; j++) {
      __cs149_mask has_to_multiply;
      _cs149_vgt_int(has_to_multiply, v_count, v_zero_int, mask_y_is_not_0);
      _cs149_vmult_float(v_result, v_result, v_values, has_to_multiply);
      _cs149_vsub_int(v_count, v_count, v_one_int, has_to_multiply);
    }

    // 7. Clamp the results for the `y != 0` lanes.
    __cs149_mask needs_clamping;
    _cs149_vgt_float(needs_clamping, v_result, v_clamp_val, mask_y_is_not_0);
    _cs149_vset_float(v_result, 9.999999f, needs_clamping);

    // 8. Merge results: set result to 1.0f for lanes where exponent was 0.
    _cs149_vset_float(v_result, 1.f, mask_y_is_0);

    // 9. Store the final combined result.
    _cs149_vstore_float(output + i, v_result, active_mask);
  }
}

// returns the sum of all elements in values
float arraySumSerial(float *values, int N) {
  float sum = 0;
  for (int i = 0; i < N; i++) {
    sum += values[i];
  }

  return sum;
}

// returns the sum of all elements in values
// You can assume N is a multiple of VECTOR_WIDTH
// You can assume VECTOR_WIDTH is a power of 2
float arraySumVector(float *values, int N) {

  //
  // CS149 STUDENTS TODO: Implement your vectorized version of arraySumSerial here
  //

  for (int i = 0; i < N; i += VECTOR_WIDTH) {
  }

  return 0.0;
}
