/* libSoX static formats list   (c) 2006-9 Chris Bagwell and SoX contributors
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

/*-------------------------- Static format handlers --------------------------*/

  FORMAT(aifc)
  FORMAT(aiff)
  FORMAT(al)
  FORMAT(au)
  FORMAT(avr)
  FORMAT(cdr)
  FORMAT(cvsd)
  FORMAT(cvu)
  FORMAT(dat)
  FORMAT(dvms)
  FORMAT(f4)
  FORMAT(f8)
  FORMAT(gsrt)
  FORMAT(hcom)
  FORMAT(htk)
  FORMAT(ima)
  FORMAT(la)
  FORMAT(lu)
  FORMAT(maud)
  FORMAT(nul)
  FORMAT(prc)
  FORMAT(raw)
  FORMAT(s1)
  FORMAT(s2)
  FORMAT(s3)
  FORMAT(s4)
  FORMAT(sf)
  FORMAT(sln)
  FORMAT(smp)
  FORMAT(sounder)
  FORMAT(soundtool)
  FORMAT(sox)
  FORMAT(sphere)
  FORMAT(svx)
  FORMAT(txw)
  FORMAT(u1)
  FORMAT(u2)
  FORMAT(u3)
  FORMAT(u4)
  FORMAT(ul)
  FORMAT(voc)
  FORMAT(vox)
  FORMAT(wav)
  FORMAT(wve)
  FORMAT(xa)

/*--------------------- Plugin or static format handlers ---------------------*/

#if defined HAVE_ALSA && (defined STATIC_ALSA || !defined HAVE_LIBLTDL)
  FORMAT(alsa)
#endif
#if defined HAVE_AMRNB && (defined STATIC_AMRNB || !defined HAVE_LIBLTDL)
  FORMAT(amr_nb)
#endif
#if defined HAVE_AMRWB && (defined STATIC_AMRWB || !defined HAVE_LIBLTDL)
  FORMAT(amr_wb)
#endif
#if defined HAVE_AO && (defined STATIC_AO || !defined HAVE_LIBLTDL)
  FORMAT(ao)
#endif
#if defined HAVE_COREAUDIO && (defined STATIC_COREAUDIO || !defined HAVE_LIBLTDL)
  FORMAT(coreaudio)
#endif
#if defined HAVE_FLAC && (defined STATIC_FLAC || !defined HAVE_LIBLTDL)
  FORMAT(flac)
#endif
#if defined HAVE_GSM && (defined STATIC_GSM || !defined HAVE_LIBLTDL)
  FORMAT(gsm)
#endif
#if defined HAVE_LPC10 && (defined STATIC_LPC10 || !defined HAVE_LIBLTDL)
  FORMAT(lpc10)
#endif
#if defined HAVE_MP3 && (defined STATIC_MP3 || !defined HAVE_LIBLTDL)
  FORMAT(mp3)
#endif
#if defined HAVE_OPUS && (defined STATIC_OPUS || !defined HAVE_LIBLTDL)
  FORMAT(opus)
#endif
#if defined HAVE_OSS && (defined STATIC_OSS || !defined HAVE_LIBLTDL)
  FORMAT(oss)
#endif
#if defined HAVE_PULSEAUDIO && (defined STATIC_PULSEAUDIO || !defined HAVE_LIBLTDL)
  FORMAT(pulseaudio)
#endif
#if defined HAVE_WAVEAUDIO && (defined STATIC_WAVEAUDIO || !defined HAVE_LIBLTDL)
  FORMAT(waveaudio)
#endif
#if defined HAVE_SNDIO && (defined STATIC_SNDIO || !defined HAVE_LIBLTDL)
  FORMAT(sndio)
#endif
#if defined HAVE_SNDFILE && (defined STATIC_SNDFILE || !defined HAVE_LIBLTDL)
  FORMAT(sndfile)
  FORMAT(caf)
  FORMAT(fap)
  FORMAT(mat4)
  FORMAT(mat5)
  FORMAT(paf)
  FORMAT(pvf)
  FORMAT(sd2)
  FORMAT(w64)
  FORMAT(xi)
#endif
#if defined HAVE_SUNAUDIO && (defined STATIC_SUNAUDIO || !defined HAVE_LIBLTDL)
  FORMAT(sunau)
#endif
#if defined HAVE_OGGVORBIS && (defined STATIC_OGGVORBIS || !defined HAVE_LIBLTDL)
  FORMAT(vorbis)
#endif
#if defined HAVE_WAVPACK && (defined STATIC_WAVPACK || !defined HAVE_LIBLTDL)
  FORMAT(wavpack)
#endif
