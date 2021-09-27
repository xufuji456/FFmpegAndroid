/* libSoX effect: Delay one or more channels (c) 2008 robs@users.sourceforge.net
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

#include "sox_i.h"
#include <string.h>

typedef struct {
  size_t argc;
  struct { char *str; uint64_t delay; } *args;
  uint64_t *max_delay;
  uint64_t delay, pre_pad, pad;
  size_t buffer_size, buffer_index;
  sox_sample_t * buffer;
  sox_bool drain_started;
} priv_t;

static int lsx_kill(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  unsigned i;

  for (i = 0; i < p->argc; ++i)
    free(p->args[i].str);
  free(p->args);
  free(p->max_delay);
  return SOX_SUCCESS;
}

static int create(sox_effect_t * effp, int argc, char * * argv)
{
  priv_t * p = (priv_t *)effp->priv;
  unsigned i;

  --argc, ++argv;
  p->argc = argc;
  p->args = lsx_calloc(p->argc, sizeof(*p->args));
  p->max_delay = lsx_malloc(sizeof(*p->max_delay));
  for (i = 0; i < p->argc; ++i) {
    char const * next = lsx_parseposition(0., p->args[i].str = lsx_strdup(argv[i]), NULL, (uint64_t)0, (uint64_t)0, '=');
    if (!next || *next) {
      lsx_kill(effp);
      return lsx_usage(effp);
    }
  }
  return SOX_SUCCESS;
}

static int stop(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  free(p->buffer);
  return SOX_SUCCESS;
}

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  uint64_t max_delay = 0, last_seen = 0, delay;
  uint64_t in_length = effp->in_signal.length != SOX_UNKNOWN_LEN ?
    effp->in_signal.length / effp->in_signal.channels : SOX_UNKNOWN_LEN;

  if (effp->flow == 0) {
    unsigned i;
    if (p->argc > effp->in_signal.channels) {
      lsx_fail("too few input channels");
      return SOX_EOF;
    }
    for (i = 0; i < p->argc; ++i) {
      if (!lsx_parseposition(effp->in_signal.rate, p->args[i].str, &delay, last_seen, in_length, '=') || delay == SOX_UNKNOWN_LEN) {
        lsx_fail("Position relative to end of audio specified, but audio length is unknown");
        return SOX_EOF;
      }
      p->args[i].delay = last_seen = delay;
      if (delay > max_delay) {
        max_delay = delay;
      }
    }
    *p->max_delay = max_delay;
    if (max_delay == 0)
      return SOX_EFF_NULL;
    effp->out_signal.length = effp->in_signal.length != SOX_UNKNOWN_LEN ?
       effp->in_signal.length + max_delay * effp->in_signal.channels :
       SOX_UNKNOWN_LEN;
    lsx_debug("extending audio by %" PRIu64 " samples", max_delay);
  }

  max_delay = *p->max_delay;
  if (effp->flow < p->argc)
    p->buffer_size = p->args[effp->flow].delay;
  p->buffer_index = p->delay = p->pre_pad = 0;
  p->pad = max_delay - p->buffer_size;
  p->buffer = lsx_malloc(p->buffer_size * sizeof(*p->buffer));
  p->drain_started = sox_false;
  return SOX_SUCCESS;
}

static int flow(sox_effect_t * effp, const sox_sample_t * ibuf,
    sox_sample_t * obuf, size_t * isamp, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t len = *isamp = *osamp = min(*isamp, *osamp);

  if (!p->buffer_size)
    memcpy(obuf, ibuf, len * sizeof(*obuf));
  else for (; len; --len) {
    if (p->delay < p->buffer_size) {
      p->buffer[p->delay++] = *ibuf++;
      *obuf++ = 0;
    } else {
      *obuf++ = p->buffer[p->buffer_index];
      p->buffer[p->buffer_index++] = *ibuf++;
      p->buffer_index %= p->buffer_size;
    }
  }
  return SOX_SUCCESS;
}

static int drain(sox_effect_t * effp, sox_sample_t * obuf, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t len;
  if (! p->drain_started) {
    p->drain_started = sox_true;
    p->pre_pad = p->buffer_size - p->delay;
      /* If the input was too short to fill the buffer completely,
         flow() has not yet output enough silence to reach the
         desired delay. */
  }
  len = *osamp = min(p->pre_pad + p->delay + p->pad, *osamp);

  for (; p->pre_pad && len; --p->pre_pad, --len)
    *obuf++ = 0;
  for (; p->delay && len; --p->delay, --len) {
    *obuf++ = p->buffer[p->buffer_index++];
    p->buffer_index %= p->buffer_size;
  }
  for (; p->pad && len; --p->pad, --len)
    *obuf++ = 0;
  return SOX_SUCCESS;
}

sox_effect_handler_t const * lsx_delay_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "delay", "{position}", SOX_EFF_LENGTH | SOX_EFF_MODIFY,
    create, start, flow, drain, stop, lsx_kill, sizeof(priv_t)
  };
  return &handler;
}
