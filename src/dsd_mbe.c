/*
 * Copyright (C) 2010 DSD Author
 * GPG Key ID: 0x3F1D7FD0 (74EF 430D F7F2 0A48 FCE6  F630 FAA2 635D 3F1D 7FD0)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "dsd.h"


static void DecipherData(char * Input, char * KeyStream, char * Output, int NbData);

void
playMbeFiles (dsd_opts * opts, dsd_state * state, int argc, char **argv)
{
#ifdef USE_MBE
  int i;
  char imbe_d[88];
  char ambe_d[49];

  for (i = state->optind; i < argc; i++)
    {
      sprintf(opts->mbe_in_file, "%s", argv[i]);
      openMbeInFile (opts, state);
#ifdef USE_MBE
      mbe_initMbeParms (state->cur_mp, state->prev_mp, state->prev_mp_enhanced);
#endif
      fprintf(stderr, "playing %s\n", opts->mbe_in_file);
      while (feof (opts->mbe_in_f) == 0)
        {
          if (state->mbe_file_type == 0)
            {
              readImbe4400Data (opts, state, imbe_d);

              mbe_processImbe4400Dataf (state->audio_out_temp_buf, &state->errs, &state->errs2, state->err_str, imbe_d, state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);
              processAudio (opts, state);
              if (opts->wav_out_f != NULL)
                {
                  writeSynthesizedVoice (opts, state);
                }

              if (opts->audio_out == 1)
                {
                  playSynthesizedVoice (opts, state);
                }
            }
          else if (state->mbe_file_type == 1)
            {
              readAmbe2450Data (opts, state, ambe_d);
#ifdef USE_MBE
              mbe_processAmbe2450Dataf (state->audio_out_temp_buf, &state->errs, &state->errs2, state->err_str, ambe_d, state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);
#endif
              processAudio (opts, state);
              if (opts->wav_out_f != NULL)
                {
                  writeSynthesizedVoice (opts, state);
                }
              if (opts->audio_out == 1)
                {
                  playSynthesizedVoice (opts, state);
                }
            }
          if (exitflag == 1)
            {
              cleanupAndExit (opts, state);
            }
        }
    }
#endif
}

void
processMbeFrame (dsd_opts * opts, dsd_state * state, char imbe_fr[8][23], char ambe_fr[4][24], char imbe7100_fr[7][24])
{

  int i;
  char imbe_d[88];
  char ambe_d[49];
#ifdef AMBE_PACKET_OUT
  char ambe_d_str[50];
#endif
#ifdef USE_MBE
  for (i = 0; i < 88; i++)
    {
      imbe_d[i] = 0;
    }

  if ((state->synctype == 0) || (state->synctype == 1))
    {
      //  0 +P25p1
      //  1 -P25p1

      mbe_processImbe7200x4400Framef (state->audio_out_temp_buf, &state->errs, &state->errs2, state->err_str, imbe_fr, imbe_d, state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);
      if (opts->mbe_out_f != NULL)
        {
          saveImbe4400Data (opts, state, imbe_d);
        }
    }
  else if ((state->synctype == 14) || (state->synctype == 15))
    {
      mbe_processImbe7100x4400Framef (state->audio_out_temp_buf, &state->errs, &state->errs2, state->err_str, imbe7100_fr, imbe_d, state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);
      if (opts->mbe_out_f != NULL)
        {
          saveImbe4400Data (opts, state, imbe_d);
        }
    }
  else if ((state->synctype == 6) || (state->synctype == 7))
    {
      mbe_processAmbe3600x2400Framef (state->audio_out_temp_buf, &state->errs, &state->errs2, state->err_str, ambe_fr, ambe_d, state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);
      if (opts->mbe_out_f != NULL)
        {
          saveAmbe2450Data (opts, state, ambe_d);
        }
    }
  else
    {
      mbe_processAmbe3600x2450Framef (state->audio_out_temp_buf, &state->errs, &state->errs2, state->err_str, ambe_fr, ambe_d, state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);
#ifdef AMBE_PACKET_OUT
      for(i=0; i<49; i++) {
          ambe_d_str[i] = ambe_d[i] + '0';
      }
      ambe_d_str[49] = '\0';
      // print binary string
      fprintf(stderr, "\n?\t?\t%s\t", ambe_d_str);
      // print error data
      fprintf(stderr, "E1: %d; E2: %d; S: %s", state->errs, state->errs2, state->err_str);
#endif
      if (opts->mbe_out_f != NULL)
        {
          saveAmbe2450Data (opts, state, ambe_d);
        }
    }

  if (opts->errorbars == 1)
    {
      fprintf(stderr, "%s", state->err_str);
    }

  state->debug_audio_errors += state->errs2;

  processAudio (opts, state);
  if (opts->wav_out_f != NULL)
    {
      writeSynthesizedVoice (opts, state);
    }

  if (opts->audio_out == 1)
    {
      playSynthesizedVoice (opts, state);
    }
#endif
}


void processMbeFrameEncrypted (dsd_opts * opts, dsd_state * state, char imbe_fr[8][23], char ambe_fr[4][24], char imbe7100_fr[7][24], char ambe_keystream[49], char imbe_keystream[88])
{

  int i;
  char imbe_d[88];
  char ambe_d[49];
#ifdef AMBE_PACKET_OUT
  char ambe_d_str[50];
#endif

  for (i = 0; i < 88; i++)
    {
      imbe_d[i] = 0;
    }
#ifdef USE_MBE
  if ((state->synctype == 0) || (state->synctype == 1))
    {
      //  0 +P25p1
      //  1 -P25p1

      mbe_processImbe7200x4400Framef (state->audio_out_temp_buf, &state->errs, &state->errs2, state->err_str, imbe_fr, imbe_d, state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);

      DecipherData(imbe_d, imbe_keystream, imbe_d, 88);

      if (opts->mbe_out_f != NULL)
        {
          saveImbe4400Data (opts, state, imbe_d);
        }
    }
  else if ((state->synctype == 14) || (state->synctype == 15))
    {
      mbe_processImbe7100x4400Framef (state->audio_out_temp_buf, &state->errs, &state->errs2, state->err_str, imbe7100_fr, imbe_d, state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);

      DecipherData(imbe_d, imbe_keystream, imbe_d, 88);

      if (opts->mbe_out_f != NULL)
        {
          saveImbe4400Data (opts, state, imbe_d);
        }
    }
  else if ((state->synctype == 6) || (state->synctype == 7))
    {
      //mbe_processAmbe3600x2400Framef (state->audio_out_temp_buf, &state->errs, &state->errs2, state->err_str, ambe_fr, ambe_d, state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);

      state->errs = 0;
      state->errs2 = 0;
      state->errs = mbe_eccAmbe3600x2400C0 (ambe_fr);
      mbe_demodulateAmbe3600x2400Data (ambe_fr);
      state->errs2 = state->errs;
      state->errs2 += mbe_eccAmbe3600x2400Data (ambe_fr, ambe_d);

      memcpy(state->ambe_ciphered, ambe_d, sizeof(state->ambe_ciphered));
      DecipherData(ambe_d, ambe_keystream, ambe_d, 49);
      memcpy(state->ambe_deciphered, ambe_d, sizeof(state->ambe_deciphered));

      mbe_processAmbe2400Dataf (state->audio_out_temp_buf, &state->errs, &state->errs2, state->err_str, ambe_d, state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);

      if (opts->mbe_out_f != NULL)
        {
          saveAmbe2450Data (opts, state, ambe_d);
        }
    }
  else
    {
      //mbe_processAmbe3600x2450Framef (state->audio_out_temp_buf, &state->errs, &state->errs2, state->err_str, ambe_fr, ambe_d, state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);

      state->errs = 0;
      state->errs2 = 0;
#ifdef USE_MBE
      state->errs = mbe_eccAmbe3600x2450C0 (ambe_fr);
      mbe_demodulateAmbe3600x2450Data (ambe_fr);
      state->errs2 = state->errs;
      state->errs2 += mbe_eccAmbe3600x2450Data (ambe_fr, ambe_d);

      memcpy(state->ambe_ciphered, ambe_d, sizeof(state->ambe_ciphered));
      DecipherData(ambe_d, ambe_keystream, ambe_d, 49);
      memcpy(state->ambe_deciphered, ambe_d, sizeof(state->ambe_deciphered));

      mbe_processAmbe2450Dataf (state->audio_out_temp_buf, &state->errs, &state->errs2, state->err_str, ambe_d, state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);
#endif

#ifdef AMBE_PACKET_OUT
      for(i=0; i<49; i++) {
          ambe_d_str[i] = ambe_d[i] + '0';
      }
      ambe_d_str[49] = '\0';
      // print binary string
      fprintf(stderr, "\n?\t?\t%s\t", ambe_d_str);
      // print error data
      fprintf(stderr, "E1: %d; E2: %d; S: %s", state->errs, state->errs2, state->err_str);
#endif
      if (opts->mbe_out_f != NULL)
        {
          saveAmbe2450Data (opts, state, ambe_d);
        }
    }
#endif
  if (opts->errorbars == 1)
    {
      fprintf(stderr, "%s", state->err_str);
    }

  state->debug_audio_errors += state->errs2;

  processAudio (opts, state);
  if (opts->wav_out_f != NULL)
    {
      writeSynthesizedVoice (opts, state);
    }

  if (opts->audio_out == 1)
    {
      playSynthesizedVoice (opts, state);
    }
}


/* This function decipher/decrypt data */
static void DecipherData(char * Input, char * KeyStream, char * Output, int NbData)
{
  int i;

  for(i = 0; i < NbData; i++)
  {
    Output[i] = Input[i] ^ KeyStream[i];
  }
}

