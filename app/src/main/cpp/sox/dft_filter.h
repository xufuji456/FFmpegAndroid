#include "fft4g.h"
#define  FIFO_SIZE_T int
#include "fifo.h"

typedef struct {
  int        dft_length, num_taps, post_peak;
  double     * coefs;
} dft_filter_t;

typedef struct {
  uint64_t   samples_in, samples_out;
  fifo_t     input_fifo, output_fifo;
  dft_filter_t   filter, * filter_ptr;
} dft_filter_priv_t;

void lsx_set_dft_filter(dft_filter_t * f, double * h, int n, int post_peak);
