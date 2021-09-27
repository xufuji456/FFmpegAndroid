/* Effect: dither/noise-shape   Copyright (c) 2008-9 robs@users.sourceforge.net
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef NDEBUG /* Enable assert always. */
#undef NDEBUG /* Must undef above assert.h or other that might include it. */
#endif

#include "sox_i.h"
#include <assert.h>

#undef RANQD1
#define RANQD1 ranqd1(p->ranqd1)

typedef enum { /* Collection of various filters from the net */
  Shape_none, Shape_lipshitz, Shape_f_weighted, Shape_modified_e_weighted,
  Shape_improved_e_weighted, Shape_gesemann, Shape_shibata, Shape_low_shibata, Shape_high_shibata
} filter_name_t;
static lsx_enum_item const filter_names[] = {
  LSX_ENUM_ITEM(Shape_,none)
  LSX_ENUM_ITEM(Shape_,lipshitz)
  {"f-weighted", Shape_f_weighted},
  {"modified-e-weighted", Shape_modified_e_weighted},
  {"improved-e-weighted", Shape_improved_e_weighted},
  LSX_ENUM_ITEM(Shape_,gesemann)
  LSX_ENUM_ITEM(Shape_,shibata)
  {"low-shibata", Shape_low_shibata},
  {"high-shibata", Shape_high_shibata},
  {0, 0}};

typedef struct {
  sox_rate_t  rate;
  enum {fir, iir} type;
  size_t len;
  int gain_cB; /* Chosen so clips are few if any, but not guaranteed none. */
  double const * coefs;
  filter_name_t name;
} filter_t;

static double const lip44[] = {2.033, -2.165, 1.959, -1.590, .6149};
static double const fwe44[] = {
  2.412, -3.370, 3.937, -4.174, 3.353, -2.205, 1.281, -.569, .0847};
static double const mew44[] = {
  1.662, -1.263, .4827, -.2913, .1268, -.1124, .03252, -.01265, -.03524};
static double const iew44[] = {
  2.847, -4.685, 6.214, -7.184, 6.639, -5.032, 3.263, -1.632, .4191};
static double const ges44[] = {
  2.2061, -.4706, -.2534, -.6214, 1.0587, .0676, -.6054, -.2738};
static double const ges48[] = {
  2.2374, -.7339, -.1251, -.6033, .903, .0116, -.5853, -.2571};

static double const shi48[] = {
  2.8720729351043701172,  -5.0413231849670410156,   6.2442994117736816406,
  -5.8483986854553222656, 3.7067542076110839844,  -1.0495119094848632812,
  -1.1830236911773681641,   2.1126792430877685547, -1.9094531536102294922,
  0.99913084506988525391, -0.17090806365013122559, -0.32615602016448974609,
  0.39127644896507263184, -0.26876461505889892578,  0.097676105797290802002,
  -0.023473845794796943665,
};
static double const shi44[] = {
  2.6773197650909423828,  -4.8308925628662109375,   6.570110321044921875,
  -7.4572014808654785156, 6.7263274192810058594,  -4.8481650352478027344,
  2.0412089824676513672,   0.7006359100341796875, -2.9537565708160400391,
  4.0800385475158691406,  -4.1845216751098632812,   3.3311812877655029297,
  -2.1179926395416259766,   0.879302978515625,      -0.031759146600961685181,
  -0.42382788658142089844, 0.47882103919982910156, -0.35490813851356506348,
  0.17496839165687561035, -0.060908168554306030273,
};
static double const shi38[] = {
  1.6335992813110351562,  -2.2615492343902587891,   2.4077029228210449219,
  -2.6341717243194580078, 2.1440362930297851562,  -1.8153258562088012695,
  1.0816224813461303711,  -0.70302653312683105469, 0.15991993248462677002,
  0.041549518704414367676, -0.29416576027870178223,  0.2518316805362701416,
  -0.27766478061676025391,  0.15785403549671173096, -0.10165894031524658203,
  0.016833892092108726501,
};
static double const shi32[] =
{ /* dmaker 32000: bestmax=4.99659 (inverted) */
0.82118552923202515,
-1.0063692331314087,
0.62341964244842529,
-1.0447187423706055,
0.64532512426376343,
-0.87615132331848145,
0.52219754457473755,
-0.67434263229370117,
0.44954317808151245,
-0.52557498216629028,
0.34567299485206604,
-0.39618203043937683,
0.26791760325431824,
-0.28936097025871277,
0.1883765310049057,
-0.19097308814525604,
0.10431359708309174,
-0.10633844882249832,
0.046832218766212463,
-0.039653312414884567,
};
static double const shi22[] =
{ /* dmaker 22050: bestmax=5.77762 (inverted) */
0.056581053882837296,
-0.56956905126571655,
-0.40727734565734863,
-0.33870288729667664,
-0.29810553789138794,
-0.19039161503314972,
-0.16510021686553955,
-0.13468159735202789,
-0.096633769571781158,
-0.081049129366874695,
-0.064953058958053589,
-0.054459091275930405,
-0.043378707021474838,
-0.03660014271736145,
-0.026256965473294258,
-0.018786206841468811,
-0.013387725688517094,
-0.0090983230620622635,
-0.0026585909072309732,
-0.00042083300650119781,
};
static double const shi16[] =
{ /* dmaker 16000: bestmax=5.97128 (inverted) */
-0.37251132726669312,
-0.81423574686050415,
-0.55010956525802612,
-0.47405767440795898,
-0.32624706625938416,
-0.3161766529083252,
-0.2286367267370224,
-0.22916607558727264,
-0.19565616548061371,
-0.18160104751586914,
-0.15423151850700378,
-0.14104481041431427,
-0.11844276636838913,
-0.097583092749118805,
-0.076493598520755768,
-0.068106919527053833,
-0.041881654411554337,
-0.036922425031661987,
-0.019364040344953537,
-0.014994367957115173,
};
static double const shi11[] =
{ /* dmaker 11025: bestmax=5.9406 (inverted) */
-0.9264228343963623,
-0.98695987462997437,
-0.631156325340271,
-0.51966935396194458,
-0.39738872647285461,
-0.35679301619529724,
-0.29720726609230042,
-0.26310476660728455,
-0.21719355881214142,
-0.18561814725399017,
-0.15404847264289856,
-0.12687471508979797,
-0.10339745879173279,
-0.083688631653785706,
-0.05875682458281517,
-0.046893671154975891,
-0.027950936928391457,
-0.020740609616041183,
-0.009366452693939209,
-0.0060260160826146603,
};
static double const shi08[] =
{ /* dmaker 8000: bestmax=5.56234 (inverted) */
-1.202863335609436,
-0.94103097915649414,
-0.67878556251525879,
-0.57650017738342285,
-0.50004476308822632,
-0.44349345564842224,
-0.37833768129348755,
-0.34028723835945129,
-0.29413089156150818,
-0.24994957447052002,
-0.21715600788593292,
-0.18792112171649933,
-0.15268312394618988,
-0.12135542929172516,
-0.099610626697540283,
-0.075273610651493073,
-0.048787496984004974,
-0.042586319148540497,
-0.028991291299462318,
-0.011869125068187714,
};
static double const shl48[] = {
  2.3925774097442626953,  -3.4350297451019287109,   3.1853709220886230469,
  -1.8117271661758422852, -0.20124770700931549072,  1.4759907722473144531,
  -1.7210904359817504883,   0.97746700048446655273, -0.13790138065814971924,
  -0.38185903429985046387,  0.27421241998672485352,  0.066584214568138122559,
  -0.35223302245140075684,  0.37672343850135803223, -0.23964276909828186035,
  0.068674825131893157959,
};
static double const shl44[] = {
  2.0833916664123535156,  -3.0418450832366943359,   3.2047898769378662109,
  -2.7571926116943359375, 1.4978630542755126953,  -0.3427594602108001709,
  -0.71733748912811279297,  1.0737057924270629883, -1.0225815773010253906,
  0.56649994850158691406, -0.20968692004680633545, -0.065378531813621520996,
  0.10322438180446624756, -0.067442022264003753662, -0.00495197344571352005,
};
static double const shh44[] = {
   3.0259189605712890625, -6.0268716812133789062,   9.195003509521484375,
   -11.824929237365722656, 12.767142295837402344, -11.917946815490722656,
   9.1739168167114257812,  -5.3712320327758789062, 1.1393624544143676758,
   2.4484779834747314453,  -4.9719839096069335938,   6.0392003059387207031,
   -5.9359521865844726562,  4.903278350830078125,   -3.5527443885803222656,
   2.1909697055816650391, -1.1672389507293701172,  0.4903914332389831543,
   -0.16519790887832641602,  0.023217858746647834778,
};

static const filter_t filters[] = {
  {44100, fir,  5, 210, lip44, Shape_lipshitz},
  {46000, fir,  9, 276, fwe44, Shape_f_weighted},
  {46000, fir,  9, 160, mew44, Shape_modified_e_weighted},
  {46000, fir,  9, 321, iew44, Shape_improved_e_weighted},
  {48000, iir,  4, 220, ges48, Shape_gesemann},
  {44100, iir,  4, 230, ges44, Shape_gesemann},
  {48000, fir, 16, 301, shi48, Shape_shibata},
  {44100, fir, 20, 333, shi44, Shape_shibata},
  {37800, fir, 16, 240, shi38, Shape_shibata},
  {32000, fir, 20, 240/*TBD*/, shi32, Shape_shibata},
  {22050, fir, 20, 240/*TBD*/, shi22, Shape_shibata},
  {16000, fir, 20, 240/*TBD*/, shi16, Shape_shibata},
  {11025, fir, 20, 240/*TBD*/, shi11, Shape_shibata},
  { 8000, fir, 20, 240/*TBD*/, shi08, Shape_shibata},
  {48000, fir, 16, 250, shl48, Shape_low_shibata},
  {44100, fir, 15, 250, shl44, Shape_low_shibata},
  {44100, fir, 20, 383, shh44, Shape_high_shibata},
  {    0, fir,  0,   0,  NULL, Shape_none},
};

#define MAX_N 20

typedef struct {
  filter_name_t filter_name;
  sox_bool      auto_detect, alt_tpdf;
  double        dummy;

  double        previous_errors[MAX_N * 2];
  double        previous_outputs[MAX_N * 2];
  size_t        pos, prec;
  uint64_t      num_output;
  int32_t       history, ranqd1, r;
  double const  * coefs;
  sox_bool      dither_off;
  sox_effect_handler_flow flow;
} priv_t;

#define CONVOLVE _ _ _ _
#define NAME flow_iir_4
#define IIR
#define N 4
#include "dither.h"
#undef IIR
#define CONVOLVE _ _ _ _ _
#define NAME flow_fir_5
#define N 5
#include "dither.h"
#define CONVOLVE _ _ _ _ _ _ _ _ _
#define NAME flow_fir_9
#define N 9
#include "dither.h"
#define CONVOLVE _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
#define NAME flow_fir_15
#define N 15
#include "dither.h"
#define CONVOLVE _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
#define NAME flow_fir_16
#define N 16
#include "dither.h"
#define CONVOLVE _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
#define NAME flow_fir_20
#define N 20
#include "dither.h"

static int flow_no_shape(sox_effect_t * effp, const sox_sample_t * ibuf,
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
        lsx_debug("flow %" PRIuPTR ": off @ %" PRIu64, effp->flow, p->num_output);
      }
    }

    if (!p->dither_off) {
      int32_t r = RANQD1 >> p->prec;
      double d = ((double)*ibuf++ + r + (p->alt_tpdf? -p->r : (RANQD1 >> p->prec))) / (1 << (32 - p->prec));
      int i = d < 0? d - .5 : d + .5;
      p->r = r;
      if (i <= (-1 << (p->prec-1)))
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

static int getopts(sox_effect_t * effp, int argc, char * * argv)
{
  priv_t * p = (priv_t *)effp->priv;
  int c;
  lsx_getopt_t optstate;
  lsx_getopt_init(argc, argv, "+aSsf:p:", NULL, lsx_getopt_flag_none, 1, &optstate);

  while ((c = lsx_getopt(&optstate)) != -1) switch (c) {
    case 'a': p->auto_detect = sox_true; break;
    case 'S': p->alt_tpdf = sox_true; break;
    case 's': p->filter_name = Shape_shibata; break;
    case 'f':
      p->filter_name = lsx_enum_option(c, optstate.arg, filter_names);
      if (p->filter_name == INT_MAX)
        return SOX_EOF;
      break;
    GETOPT_NUMERIC(optstate, 'p', prec, 1, 24)
    default: lsx_fail("invalid option `-%c'", optstate.opt); return lsx_usage(effp);
  }
  argc -= optstate.ind, argv += optstate.ind;
  return argc? lsx_usage(effp) : SOX_SUCCESS;
}

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  double mult = 1; /* Amount the noise shaping multiplies up the TPDF (+/-1) */

  if (p->prec == 0)
    p->prec = effp->out_signal.precision;

  if (effp->in_signal.precision <= p->prec || p->prec > 24)
    return SOX_EFF_NULL;   /* Dithering not needed at this resolution */

  if (p->prec == 1) {
    /* The general dither routines don't work in this case, so notify
       user and leave it at that for now.
       TODO: Some special-case treatment of 1-bit noise shaping will be
         needed for meaningful DSD write support. */
    lsx_warn("Dithering/noise-shaping to 1 bit is currently not supported.");
    return SOX_EFF_NULL;
  }

  effp->out_signal.precision = p->prec;

  p->flow = flow_no_shape;
  if (p->filter_name) {
    filter_t const * f;

    for (f = filters; f->len && (f->name != p->filter_name || fabs(effp->in_signal.rate - f->rate) / f->rate > .05); ++f); /* 5% leeway on frequency */
    if (!f->len) {
      p->alt_tpdf |= effp->in_signal.rate >= 22050;
      if (!effp->flow)
        lsx_warn("no `%s' filter is available for rate %g; using %s TPDF",
            lsx_find_enum_value(p->filter_name, filter_names)->text,
            effp->in_signal.rate, p->alt_tpdf? "sloped" : "plain");
    }
    else {
      assert(f->len <= MAX_N);
      if (f->type == fir) switch(f->len) {
        case  5: p->flow = flow_fir_5 ; break;
        case  9: p->flow = flow_fir_9 ; break;
        case 15: p->flow = flow_fir_15; break;
        case 16: p->flow = flow_fir_16; break;
        case 20: p->flow = flow_fir_20; break;
        default: assert(sox_false);
      } else switch(f->len) {
        case  4: p->flow = flow_iir_4 ; break;
        default: assert(sox_false);
      }
      p->coefs = f->coefs;
      mult = dB_to_linear(f->gain_cB * 0.1);
    }
  }
  p->ranqd1 = ranqd1(sox_globals.ranqd1) + effp->flow;
  if (effp->in_signal.mult) /* (Takes account of ostart mult (sox.c). */
    *effp->in_signal.mult *= (SOX_SAMPLE_MAX - (1 << (31 - p->prec)) *
        (2 * mult + 1)) / (SOX_SAMPLE_MAX - (1 << (31 - p->prec)));
  return SOX_SUCCESS;
}

static int flow(sox_effect_t * effp, const sox_sample_t * ibuf,
    sox_sample_t * obuf, size_t * isamp, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  return p->flow(effp, ibuf, obuf, isamp, osamp);
}

sox_effect_handler_t const * lsx_dither_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "dither", "[-S|-s|-f filter] [-a] [-p precision]"
    "\n  (none)   Use TPDF"
    "\n  -S       Use sloped TPDF (without noise shaping)"
    "\n  -s       Shape noise (with shibata filter)"
    "\n  -f name  Set shaping filter to one of: lipshitz, f-weighted,"
    "\n           modified-e-weighted, improved-e-weighted, gesemann,"
    "\n           shibata, low-shibata, high-shibata."
    "\n  -a       Automatically turn on & off dithering as needed (use with caution!)"
    "\n  -p bits  Override the target sample precision",
    SOX_EFF_PREC, getopts, start, flow, 0, 0, 0, sizeof(priv_t)
  };
  return &handler;
}
