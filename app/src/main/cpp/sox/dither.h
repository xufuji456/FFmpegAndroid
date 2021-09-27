#ifdef IIR
#define _ output += p->coefs[j] * p->previous_errors[p->pos + j] \
                  - p->coefs[N + j] * p->previous_outputs[p->pos + j], ++j;
#else
#define _ d -= p->coefs[j] * p->previous_errors[p->pos + j], ++j;
#endif
static int NAME(sox_effect_t * effp, const sox_sample_t * ibuf,
    sox_sample_t * obuf, size_t * isamp, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t len = *isamp = *osamp = min(*isamp, *osamp);

  while (len--) {
    if (p->auto_detect) {
      p->history = (p->history << 1) +
          !!(*ibuf & (((unsigned)-1) >> p->prec));
      if (p->history && p->dither_off) {
        p->dither_off = sox_false;
        lsx_debug("flow %" PRIuPTR ": on  @ %" PRIu64, effp->flow, p->num_output);
      } else if (!p->history && !p->dither_off) {
        p->dither_off = sox_true;
        memset(p->previous_errors, 0, sizeof(p->previous_errors));
        memset(p->previous_outputs, 0, sizeof(p->previous_outputs));
        lsx_debug("flow %" PRIuPTR ": off @ %" PRIu64, effp->flow, p->num_output);
      }
    }

    if (!p->dither_off) {
      int32_t r1 = RANQD1 >> p->prec, r2 = RANQD1 >> p->prec; /* Defer add! */
#ifdef IIR
      double d1, d, output = 0;
#else
      double d1, d = *ibuf++;
#endif 
      int i, j = 0;
      CONVOLVE
      assert(j == N);
      p->pos = p->pos? p->pos - 1 : p->pos - 1 + N;
#ifdef IIR
      d = *ibuf++ - output;
      p->previous_outputs[p->pos + N] = p->previous_outputs[p->pos] = output;
#endif
      d1 = (d + r1 + r2) / (1 << (32 - p->prec));
      i = d1 < 0? d1 - .5 : d1 + .5;
      p->previous_errors[p->pos + N] = p->previous_errors[p->pos] =
          (double)i * (1 << (32 - p->prec)) - d;
      if (i < (-1 << (p->prec-1)))
        ++effp->clips, *obuf = SOX_SAMPLE_MIN;
      else if (i > (int)SOX_INT_MAX(p->prec))
        ++effp->clips, *obuf = SOX_INT_MAX(p->prec) << (32 - p->prec);
      else *obuf = i << (32 - p->prec);
      ++obuf;
    }
    else
      *obuf++ = *ibuf++;
    ++p->num_output;
  }
  return SOX_SUCCESS;
}
#undef CONVOLVE
#undef _
#undef NAME
#undef N
