/* This library is free software; you can redistribute it and/or modify it
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

/* FIXME: generate this list automatically */

  EFFECT(allpass)
  EFFECT(band)
  EFFECT(bandpass)
  EFFECT(bandreject)
  EFFECT(bass)
  EFFECT(bend)
  EFFECT(biquad)
  EFFECT(chorus)
  EFFECT(channels)
  EFFECT(compand)
  EFFECT(contrast)
  EFFECT(dcshift)
  EFFECT(deemph)
  EFFECT(delay)
  EFFECT(dft_filter) /* abstract */
  EFFECT(dither)
  EFFECT(divide)
  EFFECT(downsample)
  EFFECT(earwax)
  EFFECT(echo)
  EFFECT(echos)
  EFFECT(equalizer)
  EFFECT(fade)
  EFFECT(fir)
  EFFECT(firfit)
  EFFECT(flanger)
  EFFECT(gain)
  EFFECT(highpass)
  EFFECT(hilbert)
  EFFECT(input)
#ifdef HAVE_LADSPA_H
  EFFECT(ladspa)
#endif
  EFFECT(loudness)
  EFFECT(lowpass)
  EFFECT(mcompand)
  EFFECT(noiseprof)
  EFFECT(noisered)
  EFFECT(norm)
  EFFECT(oops)
  EFFECT(output)
  EFFECT(overdrive)
  EFFECT(pad)
  EFFECT(phaser)
  EFFECT(pitch)
  EFFECT(rate)
  EFFECT(remix)
  EFFECT(repeat)
  EFFECT(reverb)
  EFFECT(reverse)
  EFFECT(riaa)
  EFFECT(silence)
  EFFECT(sinc)
#ifdef HAVE_PNG
  EFFECT(spectrogram)
#endif
  EFFECT(speed)
#ifdef HAVE_SPEEXDSP
  EFFECT(speexdsp)
#endif
  EFFECT(splice)
  EFFECT(stat)
  EFFECT(stats)
  EFFECT(stretch)
  EFFECT(swap)
  EFFECT(synth)
  EFFECT(tempo)
  EFFECT(treble)
  EFFECT(tremolo)
  EFFECT(trim)
  EFFECT(upsample)
  EFFECT(vad)
  EFFECT(vol)
