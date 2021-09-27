/* libSoX Bandpass effect file.     July 5, 1991
 * Copyright 1991 Lance Norskog And Sundry Contributors
 *
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Lance Norskog And Sundry Contributors are not responsible for
 * the consequences of using this software.
 *
 * Algorithm:  2nd order recursive filter.
 * Formula stolen from MUSIC56K, a toolkit of 56000 assembler stuff.
 * Quote:
 *   This is a 2nd order recursive band pass filter of the form.
 *   y(n)= a * x(n) - b * y(n-1) - c * y(n-2)
 *   where :
 *        x(n) = "IN"
 *        "OUT" = y(n)
 *        c = EXP(-2*pi*cBW/S_RATE)
 *        b = -4*c/(1+c)*COS(2*pi*cCF/S_RATE)
 *   if cSCL=2 (i.e. noise input)
 *        a = SQT(((1+c)*(1+c)-b*b)*(1-c)/(1+c))
 *   else
 *        a = SQT(1-b*b/(4*c))*(1-c)
 *   endif
 *   note :     cCF is the center frequency in Hertz
 *        cBW is the band width in Hertz
 *        cSCL is a scale factor, use 1 for pitched sounds
 *   use 2 for noise.
 *
 *
 * July 1, 1999 - Jan Paul Schmidt <jps@fundament.org>
 *
 *   This looks like the resonator band pass in SPKit. It's a
 *   second order all-pole (IIR) band-pass filter described
 *   at the pages 186 - 189 in
 *     Dodge, Charles & Jerse, Thomas A. 1985:
 *       Computer Music -- Synthesis, Composition and Performance.
 *       New York: Schirmer Books.
 *   Reference from the SPKit manual.
 */

  p->a2 = exp(-2 * M_PI * bw_Hz / effp->in_signal.rate);
  p->a1 = -4 * p->a2 / (1 + p->a2) * cos(2 * M_PI * p->fc / effp->in_signal.rate);
  p->b0 = sqrt(1 - p->a1 * p->a1 / (4 * p->a2)) * (1 - p->a2);
  if (p->filter_type == filter_BPF_SPK_N) {
    mult = sqrt(((1+p->a2) * (1+p->a2) - p->a1*p->a1) * (1-p->a2) / (1+p->a2)) / p->b0;
    p->b0 *= mult;
  }
