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
#include "dmr_const.h"
#include "p25p1_check_hdu.h"


void ProcessDMREncryption (dsd_opts * opts, dsd_state * state)
{
  uint32_t i, j, k;
  uint32_t Frame;

  /* AMBE voice frame = 49 bit = 7 byte => 3 AMBE sample by voice frame
   * => 6 voice frame by voice superframe */
  uint8_t  AmbeByteBuffer[6][3][7];
  uint8_t  AmbeBitBuffer[6][3][49]; /* 6 * 3 DMR frame * 49 bit AMBE sample = 882 bit */
  uint8_t  UseAmbeByteBuffer = 1;  /* 1 = Use "AmbeByteBuffer" ; 0 = Use "AmbeBitBuffer" */
  TimeSlotVoiceSuperFrame_t * TSVoiceSupFrame = NULL;
  int *errs;
  int *errs2;
  int PlayCurrentSlot = 1;

  /*
   * Currently encryption is not supported in this public version...
   * You can implement it by following the example below.
   */

  /* Check the current time slot */
  if(state->currentslot == 0)
  {
    TSVoiceSupFrame = &state->TS1SuperFrame;
  }
  else
  {
    TSVoiceSupFrame = &state->TS2SuperFrame;
  }

  /* Check the current time slot */
  if(state->currentslot == 0)
  {
    TSVoiceSupFrame = &state->TS1SuperFrame;

    /* Check if we need to play the current slot */
    if(opts->SlotToOnlyDecode == 2) PlayCurrentSlot = 0;
    else PlayCurrentSlot = 1;
  }
  else
  {
    TSVoiceSupFrame = &state->TS2SuperFrame;

    /* Check if we need to play the current slot */
    if(opts->SlotToOnlyDecode == 1) PlayCurrentSlot = 0;
    else PlayCurrentSlot = 1;
  }

  /* Convert all 49 bit AMBE sample into 7 bytes buffer */
  for(Frame = 0; Frame < 6; Frame++)
  {
    Convert49BitSampleInto7Bytes(TSVoiceSupFrame->TimeSlotAmbeVoiceFrame[Frame].AmbeBit[0],
                                 (char *)AmbeByteBuffer[Frame][0]);
    Convert49BitSampleInto7Bytes(TSVoiceSupFrame->TimeSlotAmbeVoiceFrame[Frame].AmbeBit[1],
                                 (char *)AmbeByteBuffer[Frame][1]);
    Convert49BitSampleInto7Bytes(TSVoiceSupFrame->TimeSlotAmbeVoiceFrame[Frame].AmbeBit[2],
                                 (char *)AmbeByteBuffer[Frame][2]);

    memcpy(AmbeBitBuffer[Frame][0], TSVoiceSupFrame->TimeSlotAmbeVoiceFrame[Frame].AmbeBit[0], 49);
    memcpy(AmbeBitBuffer[Frame][1], TSVoiceSupFrame->TimeSlotAmbeVoiceFrame[Frame].AmbeBit[1], 49);
    memcpy(AmbeBitBuffer[Frame][2], TSVoiceSupFrame->TimeSlotAmbeVoiceFrame[Frame].AmbeBit[2], 49);
  }


  /* Apply encryption here
   *
   * A DMR superframe = 6 frames x 3 AMBE voice sample of 49 bits each
   * uint8_t KeyStream[6][3][49];
   *
   *
   * 1) Initialize the "KeyStream" buffer (total 882 bits) with correct bits (depending of
   *    encryption mode used, MotoTRBO BP, Hytera BP, MotoTRBO EP, MotoTRBO AES...
   *
   *     // DMR voice frame #1
   *     KeyStream[0][0][0..48] ==> Filled with a 49 bits keystream to decode the AMBE sample #01
   *     KeyStream[0][1][0..48] ==> Filled with a 49 bits keystream to decode the AMBE sample #02
   *     KeyStream[0][2][0..48] ==> Filled with a 49 bits keystream to decode the AMBE sample #03
   *
   *     // DMR voice frame #2
   *     KeyStream[1][0][0..48] ==> Filled with a 49 bits keystream to decode the AMBE sample #04
   *     KeyStream[1][1][0..48] ==> Filled with a 49 bits keystream to decode the AMBE sample #05
   *     KeyStream[1][2][0..48] ==> Filled with a 49 bits keystream to decode the AMBE sample #06
   *
   *     // DMR voice frame #3
   *     KeyStream[2][0][0..48] ==> Filled with a 49 bits keystream to decode the AMBE sample #07
   *     KeyStream[2][1][0..48] ==> Filled with a 49 bits keystream to decode the AMBE sample #08
   *     KeyStream[2][2][0..48] ==> Filled with a 49 bits keystream to decode the AMBE sample #09
   *
   *     // DMR voice frame #4
   *     KeyStream[3][0][0..48] ==> Filled with a 49 bits keystream to decode the AMBE sample #10
   *     KeyStream[3][1][0..48] ==> Filled with a 49 bits keystream to decode the AMBE sample #11
   *     KeyStream[3][2][0..48] ==> Filled with a 49 bits keystream to decode the AMBE sample #12
   *
   *     // DMR voice frame #5
   *     KeyStream[4][0][0..48] ==> Filled with a 49 bits keystream to decode the AMBE sample #13
   *     KeyStream[4][1][0..48] ==> Filled with a 49 bits keystream to decode the AMBE sample #14
   *     KeyStream[4][2][0..48] ==> Filled with a 49 bits keystream to decode the AMBE sample #15
   *
   *     // DMR voice frame #6
   *     KeyStream[5][0][0..48] ==> Filled with a 49 bits keystream to decode the AMBE sample #16
   *     KeyStream[5][1][0..48] ==> Filled with a 49 bits keystream to decode the AMBE sample #17
   *     KeyStream[5][2][0..48] ==> Filled with a 49 bits keystream to decode the AMBE sample #18
   *
   *
   * 2) Apply a XOR between "KeyStream" and "AmbeBitBuffer[Frame][i][j]" like this code :
   *
   *    UseAmbeByteBuffer = 0; // The deciphered result will be available in the "AmbeBitBuffer" buffer
   *
   *    for(Frame = 0; Frame < 6; Frame++)
   *    {
   *      for(i = 0; i < 3; i++)
   *      {
   *        for(j = 0; j < 49; j++)
   *        {
   *          AmbeBitBuffer[Frame][i][j] = AmbeBitBuffer[Frame][i][j] ^ KeyStream[Frame][i][j];
   *        }
   *      }
   *    }
   *
   * 3) Play all AMBE decoded sample (see the "for" loop below)
   *
   * */


  if((opts->UseSpecificDmr49BitsAmbeKeyStreamUsed) && (state->display_raw_data == 0))
  {
    /* The deciphered result will be available in the "AmbeBitBuffer" buffer */
    UseAmbeByteBuffer = 0;

    /* Decipher all 49 bits AMBE sample
     * 1 DMR voice superframe = 6 DMR voice frames */
    for(Frame = 0, k = 0; Frame < 6; Frame++)
    {
      /* 1 DMR voice frame contains 3 AMBE samples */
      for(i = 0; i < 3; i++)
      {
        /* 1 AMBE sample contains 49 bits */
        for(j = 0; j < 49; j++)
        {
          /* XOR encrypted data with the key */
          AmbeBitBuffer[Frame][i][j] = (AmbeBitBuffer[Frame][i][j] ^ opts->UseSpecificDmr49BitsAmbeKeyStream[j]) & 1;
        }
      }
    }
  } /* End if((opts->UseSpecificDmr49BitsAmbeKeyStreamUsed) && (state->display_raw_data == 0)) */


  if((opts->UseSpecificDmrAmbeSuperFrameKeyStreamUsed) && (state->display_raw_data == 0))
  {
    /* The deciphered result will be available in the "AmbeBitBuffer" buffer */
    UseAmbeByteBuffer = 0;

    k = 0;

    /* Decipher all 49 bits AMBE sample
     * 1 DMR voice superframe = 6 DMR voice frames */
    for(Frame = 0, k = 0; Frame < 6; Frame++)
    {
      /* 1 DMR voice frame contains 3 AMBE samples */
      for(i = 0; i < 3; i++)
      {
        /* 1 AMBE sample contains 49 bits */
        for(j = 0; j < 49; j++)
        {
          /* XOR encrypted data with the key */
          AmbeBitBuffer[Frame][i][j] = (AmbeBitBuffer[Frame][i][j] ^ opts->UseSpecificDmrAmbeSuperFrameKeyStream[k++]) & 1;
        }
      }
    }
  } /* End if((opts->UseSpecificDmr49BitsAmbeKeyStreamUsed) && (state->display_raw_data == 0)) */


  /* Convert all 7 bytes buffer into 49 bit AMBE sample */
  for(Frame = 0; Frame < 6; Frame++)
  {
    /* Check the buffer to use */
    if(UseAmbeByteBuffer)
    {
      /* Use "AmbeByteBuffer" buffer */
      Convert7BytesInto49BitSample((char *)AmbeByteBuffer[Frame][0],
                                   TSVoiceSupFrame->TimeSlotAmbeVoiceFrame[Frame].AmbeBit[0]);
      Convert7BytesInto49BitSample((char *)AmbeByteBuffer[Frame][1],
                                   TSVoiceSupFrame->TimeSlotAmbeVoiceFrame[Frame].AmbeBit[1]);
      Convert7BytesInto49BitSample((char *)AmbeByteBuffer[Frame][2],
                                   TSVoiceSupFrame->TimeSlotAmbeVoiceFrame[Frame].AmbeBit[2]);
    }
    else
    {
      /* Use "AmbeBitBuffer" buffer */
      memcpy(TSVoiceSupFrame->TimeSlotAmbeVoiceFrame[Frame].AmbeBit[0], AmbeBitBuffer[Frame][0], 49);
      memcpy(TSVoiceSupFrame->TimeSlotAmbeVoiceFrame[Frame].AmbeBit[1], AmbeBitBuffer[Frame][1], 49);
      memcpy(TSVoiceSupFrame->TimeSlotAmbeVoiceFrame[Frame].AmbeBit[2], AmbeBitBuffer[Frame][2], 49);
    }
  }


  /*
   * Play all AMBE voice samples
   * 1 DMR voice superframe = 6 DMR frames */
  for(Frame = 0; Frame < 6; Frame++)
  {
    /* 1 DMR frame contains 3 AMBE voice samples */
    for(i = 0; i < 3; i++)
    {
      errs  = (int*)&(TSVoiceSupFrame->TimeSlotAmbeVoiceFrame[Frame].errs1[i]);
      errs2 = (int*)&(TSVoiceSupFrame->TimeSlotAmbeVoiceFrame[Frame].errs2[i]);
#ifdef USE_MBE
      mbe_processAmbe2450Dataf (state->audio_out_temp_buf, errs, errs2, state->err_str,
                                TSVoiceSupFrame->TimeSlotAmbeVoiceFrame[Frame].AmbeBit[i],
                                state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);
#endif
      if (opts->mbe_out_f != NULL)
      {
        saveAmbe2450Data (opts, state, TSVoiceSupFrame->TimeSlotAmbeVoiceFrame[Frame].AmbeBit[i]);
      }
      if (opts->errorbars == 1)
      {
        fprintf(stderr, "%s", state->err_str);
      }

      state->debug_audio_errors += *errs2;

      /* Play the current slot only if allowed */
      if(PlayCurrentSlot)
      {
        processAudio(opts, state);

        if (opts->wav_out_f != NULL)
        {
          writeSynthesizedVoice (opts, state);
        }

        if (opts->audio_out == 1)
        {
          playSynthesizedVoice (opts, state);
        }
      } /* End if(PlayCurrentSlot) */
    } /* End for(i = 0; i < 3; i++) */
  } /* End for(Frame = 0; Frame < 6; Frame++) */
} /* End ProcessDMREncryption() */



/* End of file */
