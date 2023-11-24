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

void processDMRvoice (dsd_opts * opts, dsd_state * state)
{
  // extracts AMBE frames from DMR frame
  int i, j, k, l, dibit;
  int *dibit_p;
  char ambe_fr[4][24] = {0};
  char ambe_fr2[4][24] = {0};
  char ambe_fr3[4][24] = {0};
  const int *w, *x, *y, *z;
  char sync[25];
  char syncdata[48];
  char cachdata[13] = {0};
  int mutecurrentslot;
  int msMode;
  char cc[4];
  unsigned char EmbeddedSignalling[16];
  unsigned int EmbeddedSignallingOk;
  char SlotType[20];
  unsigned int SlotTypeOk = 0;
  char bursttype[5];
  int decode_next_slot = 0;
  int interrupt_current_slot_decoding = 0;
  int voice_sync_next_slot_detected = 0;

  /* Remove warning compiler */
  UNUSED_VARIABLE(ambe_fr[0][0]);
  UNUSED_VARIABLE(ambe_fr2[0][0]);
  UNUSED_VARIABLE(ambe_fr3[0][0]);
  UNUSED_VARIABLE(cachdata[0]);

#ifdef DMR_DUMP
  int k;
  char syncbits[49];
  char cachbits[25];12
#endif

  /* Backup Link Control data that need to be saved */
  DmrBackupLinkControlData(opts, state);

  /* Init the superframe buffers */
  memset(&state->TS1SuperFrame, 0, sizeof(TimeSlotVoiceSuperFrame_t));
  memset(&state->TS2SuperFrame, 0, sizeof(TimeSlotVoiceSuperFrame_t));

  /* Restore Link control Data that need to be restored */
  DmrRestoreLinkControlData(opts, state);

  /* Init the color code status */
  state->color_code_ok = 0;

  mutecurrentslot = 0;
  msMode = 0;

  dibit_p = state->dibit_buf_p - 90;

  for(j = 0; j < 6; j++)
  {

    // CACH (24 bits / 18 dibits)
    for(i = 0; i < 12; i++)
    {
      if(j > 0)
      {
        dibit = getDibit(opts, state);
      }
      else
      {
        dibit = *dibit_p;
        dibit_p++;
        if(opts->inverted_dmr == 1)
        {
          dibit = (dibit ^ 2);
        }
      }
      cachdata[i] = dibit;
      if(i == 2)
      {
        /* Change the current slot only if we are in relayed mode (not in direct mode) */
        if(state->directmode == 0) state->currentslot = (1 & (dibit >> 1));  // bit 1

        if(state->currentslot == 0)
        {
          state->slot1light[0] = '[';
          state->slot1light[6] = ']';
          state->slot2light[0] = ' ';
          state->slot2light[6] = ' ';

          if(opts->SlotToOnlyDecode == 2) decode_next_slot = 1;
          else decode_next_slot = 0;
        }
        else
        {
          state->slot2light[0] = '[';
          state->slot2light[6] = ']';
          state->slot1light[0] = ' ';
          state->slot1light[6] = ' ';

          if(opts->SlotToOnlyDecode == 1) decode_next_slot = 1;
          else decode_next_slot = 0;
        }
      }
    }
    cachdata[12] = 0;


#ifdef DMR_DUMP
    k = 0;
    for(i = 0; i < 12; i++)
    {
      dibit = cachdata[i];
      cachbits[k] = (1 & (dibit >> 1)) + 48;        // bit 1
      k++;
      cachbits[k] = (1 & dibit) + 48;       // bit 0
      k++;
    }
    cachbits[24] = 0;
    fprintf(stderr, "%s ", cachbits);
#endif

    // Current slot AMBE frame 1/3 (72 bits / 36 dibits)
    w = rW;
    x = rX;
    y = rY;
    z = rZ;
    k = 0;
    for(i = 0; i < 36; i++)
    {
      if(j > 0)
      {
        dibit = getDibit(opts, state);
      }
      else
      {
        dibit = *dibit_p;
        dibit_p++;
        if(opts->inverted_dmr == 1)
        {
          dibit = (dibit ^ 2);
        }
      }
      ambe_fr[*w][*x] = (1 & (dibit >> 1)); // bit 1
      ambe_fr[*y][*z] = (1 & dibit);        // bit 0

      /* Save the current frame in the appropriate superframe buffer */
      if(state->currentslot == 0)
      {
        /* Time slot 1 superframe buffer filling => RAW voice bit */
        state->TS1SuperFrame.TimeSlotRawVoiceFrame[j].VoiceFrame[0][k++] = (1 & (dibit >> 1)); // bit 1
        state->TS1SuperFrame.TimeSlotRawVoiceFrame[j].VoiceFrame[0][k++] = (1 & dibit);        // bit 0

        /* Time slot 1 superframe buffer filling => Deinterleaved voice bit */
        state->TS1SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[0][*w][*x] = (1 & (dibit >> 1)); // bit 1
        state->TS1SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[0][*y][*z] = (1 & dibit);        // bit 0
      }
      else
      {
        /* Time slot 2 superframe buffer filling => RAW voice bit */
        state->TS2SuperFrame.TimeSlotRawVoiceFrame[j].VoiceFrame[0][k++] = (1 & (dibit >> 1)); // bit 1
        state->TS2SuperFrame.TimeSlotRawVoiceFrame[j].VoiceFrame[0][k++] = (1 & dibit);        // bit 0

        /* Time slot 2 superframe buffer filling => Deinterleaved voice bit */
        state->TS2SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[0][*w][*x] = (1 & (dibit >> 1)); // bit 1
        state->TS2SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[0][*y][*z] = (1 & dibit);        // bit 0
      }

      w++;
      x++;
      y++;
      z++;
    }

    // Current slot AMBE frame 2/3 (first half) [36 bits / 18 dibits]
    w = rW;
    x = rX;
    y = rY;
    z = rZ;
    k = 0;
    for(i = 0; i < 18; i++)
    {
      if(j > 0)
      {
        dibit = getDibit(opts, state);
      }
      else
      {
        dibit = *dibit_p;
        dibit_p++;
        if(opts->inverted_dmr == 1)
        {
          dibit = (dibit ^ 2);
        }
      }
      ambe_fr2[*w][*x] = (1 & (dibit >> 1)); // bit 1
      ambe_fr2[*y][*z] = (1 & dibit);        // bit 0

      /* Save the current frame in the appropriate superframe buffer */
      if(state->currentslot == 0)
      {
        /* Time slot 1 superframe buffer filling => RAW voice bit */
        state->TS1SuperFrame.TimeSlotRawVoiceFrame[j].VoiceFrame[1][k++] = (1 & (dibit >> 1)); // bit 1
        state->TS1SuperFrame.TimeSlotRawVoiceFrame[j].VoiceFrame[1][k++] = (1 & dibit);        // bit 0

        /* Time slot 1 superframe buffer filling => Deinterleaved voice bit */
        state->TS1SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[1][*w][*x] = (1 & (dibit >> 1)); // bit 1
        state->TS1SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[1][*y][*z] = (1 & dibit);        // bit 0
      }
      else
      {
        /* Time slot 2 superframe buffer filling => RAW voice bit */
        state->TS2SuperFrame.TimeSlotRawVoiceFrame[j].VoiceFrame[1][k++] = (1 & (dibit >> 1)); // bit 1
        state->TS2SuperFrame.TimeSlotRawVoiceFrame[j].VoiceFrame[1][k++] = (1 & dibit);        // bit 0

        /* Time slot 2 superframe buffer filling => Deinterleaved voice bit */
        state->TS2SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[1][*w][*x] = (1 & (dibit >> 1)); // bit 1
        state->TS2SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[1][*y][*z] = (1 & dibit);        // bit 0
      }

      w++;
      x++;
      y++;
      z++;
    }

    l = 0;

    // Signaling data or sync (48 bits - 24 dibits)
    for(i = 0; i < 24; i++)
    {
      if(j > 0)
      {
        dibit = getDibit(opts, state);
      }
      else
      {
        dibit = *dibit_p;
        dibit_p++;
        if(opts->inverted_dmr == 1)
        {
          dibit = (dibit ^ 2);
        }
      }
      syncdata[(2*i)]   = (1 & (dibit >> 1));  // bit 1
      syncdata[(2*i)+1] = (1 & dibit);         // bit 0
      sync[i] = (dibit | 1) + 48;

      /* Save the current frame in the appropriate superframe buffer */
      if(state->currentslot == 0)
      {
        /* Time slot 1 superframe buffer filling => SYNC data */
        state->TS1SuperFrame.TimeSlotRawVoiceFrame[j].Sync[l++] = (1 & (dibit >> 1)); // bit 1
        state->TS1SuperFrame.TimeSlotRawVoiceFrame[j].Sync[l++] = (1 & dibit);        // bit 0
      }
      else
      {
        /* Time slot 2 superframe buffer filling => SYNC data */
        state->TS2SuperFrame.TimeSlotRawVoiceFrame[j].Sync[l++] = (1 & (dibit >> 1)); // bit 1
        state->TS2SuperFrame.TimeSlotRawVoiceFrame[j].Sync[l++] = (1 & dibit);        // bit 0
      }
    }
    sync[24] = 0;

    /* Copy the Embedded signaling (1st part) */
    for(i = 0; i < 8; i++)
    {
      EmbeddedSignalling[i] = syncdata[i];
    }
    /* Copy the Embedded signaling (2nd part) */
    for(i = 0; i < 8; i++)
    {
      EmbeddedSignalling[i + 8] = syncdata[i + 40];
    }

    /* Check and correct the SlotType (apply Golay(20,8) FEC check) */
    if(QR_16_7_6_decode(EmbeddedSignalling)) EmbeddedSignallingOk = 1;
    else EmbeddedSignallingOk = 0;

    /* Get the color code */
    cc[0] = EmbeddedSignalling[0];
    cc[1] = EmbeddedSignalling[1];
    cc[2] = EmbeddedSignalling[2];
    cc[3] = EmbeddedSignalling[3];

    /* The color code is normally the same for all
     * 6 received voice frame, so in case of error
     * do not store it except if the last SYNC frame
     * has been received. */
    if(EmbeddedSignallingOk || ((state->color_code_ok == 0) && (j == 5)))
    {
      /* Save the color code */
      state->color_code = (unsigned int)((cc[0] << 3) + (cc[1] << 2) + (cc[2] << 1) + cc[3]);
      state->color_code_ok = EmbeddedSignallingOk;
    }

    /* Save the Power Control (PI) indicator */
    state->PI = (unsigned int)EmbeddedSignalling[4];
    state->PI_ok = EmbeddedSignallingOk;

    /* Save the Link Control Start Stop (LCSS) indicator */
    state->LCSS = (unsigned int)((EmbeddedSignalling[5] << 1) + EmbeddedSignalling[6]);
    state->LCSS_ok = EmbeddedSignallingOk;

    if(strcmp (sync, DMR_BS_DATA_SYNC) == 0)
    {
      mutecurrentslot = 1;
      state->directmode = 0;
      if(state->currentslot == 0)
      {
        sprintf(state->slot1light, "[slot1]");
      }
      else
      {
        sprintf(state->slot2light, "[slot2]");
      }
    }
    else if(strcmp (sync, DMR_MS_DATA_SYNC) == 0)
    {
      mutecurrentslot = 1;
      state->directmode = 1;  /* Direct mode */
      state->currentslot = 0; /* In direct mode unknown slot, force current slot to 0 (slot 1) */
      sprintf(state->slot1light, "[slot1]");
    }
    else if(strcmp (sync, DMR_DIRECT_MODE_TS1_DATA_SYNC) == 0)
    {
      mutecurrentslot = 1;
      state->currentslot = 0;
      state->directmode = 1; /* Direct mode */
      sprintf(state->slot1light, "[sLoT1]");
    }
    else if(strcmp (sync, DMR_DIRECT_MODE_TS2_DATA_SYNC) == 0)
    {
      mutecurrentslot = 1;
      state->currentslot = 1;
      state->directmode = 1; /* Direct mode */
      sprintf(state->slot1light, "[sLoT2]");
    }
    else if(strcmp (sync, DMR_BS_VOICE_SYNC) == 0)
    {
      mutecurrentslot = 0;
      state->directmode = 0;
      if(state->currentslot == 0)
      {
        sprintf(state->slot1light, "[SLOT1]");
      }
      else
      {
        sprintf(state->slot2light, "[SLOT2]");
      }
    }
    else if(strcmp (sync, DMR_MS_VOICE_SYNC) == 0)
    {
      mutecurrentslot = 0;
      state->directmode = 1;  /* Direct mode */
      state->currentslot = 0; /* In direct mode unknown slot, force current slot to 0 (slot 1) */
      sprintf(state->slot1light, "[SLOT1]");
    }
    else if(strcmp (sync, DMR_DIRECT_MODE_TS1_VOICE_SYNC) == 0)
    {
      mutecurrentslot = 0;
      state->currentslot = 0;
      state->directmode = 1; /* Direct mode */
      sprintf(state->slot1light, "[sLoT1]");
    }
    else if(strcmp (sync, DMR_DIRECT_MODE_TS2_VOICE_SYNC) == 0)
    {
      mutecurrentslot = 0;
      state->currentslot = 1;
      state->directmode = 1; /* Direct mode */
      sprintf(state->slot2light, "[sLoT2]");
    }

    if((strcmp (sync, DMR_MS_VOICE_SYNC) == 0) || (strcmp (sync, DMR_MS_DATA_SYNC) == 0))
    {
      msMode = 1;
    }

// TODO : To be removed
//    if((j == 0) && (opts->errorbars == 1))
//    {
//      fprintf(stderr, "%s %s  VOICE e:", state->slot1light, state->slot2light);
//    }

#ifdef DMR_DUMP
    k = 0;
    for(i = 0; i < 24; i++)
    {
      dibit = syncdata[i];
      syncbits[k] = (1 & (dibit >> 1)) + 48;        // bit 1
      k++;
      syncbits[k] = (1 & dibit) + 48;       // bit 0
      k++;
    }
    syncbits[48] = 0;
    fprintf(stderr, "%s ", syncbits);
#endif

    // Current slot AMBE frame 2/2 (second half) (36 bits / 18 dibits)
    for(i = 0; i < 18; i++)
    {
      dibit = getDibit(opts, state);
      ambe_fr2[*w][*x] = (1 & (dibit >> 1)); // bit 1
      ambe_fr2[*y][*z] = (1 & dibit);        // bit 0

      /* Save the current frame in the appropriate superframe buffer */
      if(state->currentslot == 0)
      {
        /* Time slot 1 superframe buffer filling => RAW voice bit */
        state->TS1SuperFrame.TimeSlotRawVoiceFrame[j].VoiceFrame[1][k++] = (1 & (dibit >> 1)); // bit 1
        state->TS1SuperFrame.TimeSlotRawVoiceFrame[j].VoiceFrame[1][k++] = (1 & dibit);        // bit 0

        /* Time slot 1 superframe buffer filling => Deinterleaved voice bit */
        state->TS1SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[1][*w][*x] = (1 & (dibit >> 1)); // bit 1
        state->TS1SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[1][*y][*z] = (1 & dibit);        // bit 0
      }
      else
      {
        /* Time slot 2 superframe buffer filling */
        state->TS2SuperFrame.TimeSlotRawVoiceFrame[j].VoiceFrame[1][k++] = (1 & (dibit >> 1)); // bit 1
        state->TS2SuperFrame.TimeSlotRawVoiceFrame[j].VoiceFrame[1][k++] = (1 & dibit);        // bit 0

        /* Time slot 2 superframe buffer filling => Deinterleaved voice bit */
        state->TS2SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[1][*w][*x] = (1 & (dibit >> 1)); // bit 1
        state->TS2SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[1][*y][*z] = (1 & dibit);        // bit 0
      }

      w++;
      x++;
      y++;
      z++;
    }

    if(mutecurrentslot == 0)
    {
      if(state->firstframe == 1)
      {                   // we don't know if anything received before the first sync after no carrier is valid
        state->firstframe = 0;
      }
      else
      {
        //processMbeFrame (opts, state, NULL, ambe_fr, NULL);
        //processMbeFrame (opts, state, NULL, ambe_fr2, NULL);
      }
    }

    // Current slot AMBE frame 3/3 (72 bits / 36 dibits)
    w = rW;
    x = rX;
    y = rY;
    z = rZ;
    k = 0;
    for(i = 0; i < 36; i++)
    {
      dibit = getDibit(opts, state);
      ambe_fr3[*w][*x] = (1 & (dibit >> 1)); // bit 1
      ambe_fr3[*y][*z] = (1 & dibit);        // bit 0

      /* Save the current frame in the appropriate superframe buffer */
      if(state->currentslot == 0)
      {
        /* Time slot 1 superframe buffer filling => RAW voice bit */
        state->TS1SuperFrame.TimeSlotRawVoiceFrame[j].VoiceFrame[2][k++] = (1 & (dibit >> 1)); // bit 1
        state->TS1SuperFrame.TimeSlotRawVoiceFrame[j].VoiceFrame[2][k++] = (1 & dibit);        // bit 0

        /* Time slot 1 superframe buffer filling => Deinterleaved voice bit */
        state->TS1SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[2][*w][*x] = (1 & (dibit >> 1)); // bit 1
        state->TS1SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[2][*y][*z] = (1 & dibit);        // bit 0
      }
      else
      {
        /* Time slot 2 superframe buffer filling => RAW voice bit */
        state->TS2SuperFrame.TimeSlotRawVoiceFrame[j].VoiceFrame[2][k++] = (1 & (dibit >> 1)); // bit 1
        state->TS2SuperFrame.TimeSlotRawVoiceFrame[j].VoiceFrame[2][k++] = (1 & dibit);        // bit 0

        /* Time slot 2 superframe buffer filling => Deinterleaved voice bit */
        state->TS2SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[2][*w][*x] = (1 & (dibit >> 1)); // bit 1
        state->TS2SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[2][*y][*z] = (1 & dibit);        // bit 0
      }

      w++;
      x++;
      y++;
      z++;
    }
    if(mutecurrentslot == 0)
    {
      //processMbeFrame (opts, state, NULL, ambe_fr3, NULL);
    }

    // CACH (24 bits / 18 dibits)
    for(i = 0; i < 12; i++)
    {
      dibit = getDibit(opts, state);
      cachdata[i] = dibit;
    }
    cachdata[12] = 0;

#ifdef DMR_DUMP
    k = 0;
    for(i = 0; i < 12; i++)
      {
        dibit = cachdata[i];
        cachbits[k] = (1 & (dibit >> 1)) + 48;        // bit 1
        k++;
        cachbits[k] = (1 & dibit) + 48;       // bit 0
        k++;
      }
    cachbits[24] = 0;
    fprintf(stderr, "%s ", cachbits);
#endif


    // Next slot (only 98 first bits / 49 dibits)
    skipDibit (opts, state, 49);

    // Next slot : Slot Type (first part) [10 bits / 5 dibits]
    for(i = 0; i < 5; i++)
    {
      dibit = getDibit(opts, state);
      if(opts->inverted_dmr == 1)
      {
        dibit = (dibit ^ 2);
      }
      SlotType[i * 2] = (1 & (dibit >> 1)); // bit 1
      SlotType[i * 2] = (1 & dibit);        // bit 0
    }

    // Next slot : Signaling data or sync (48 bits / 24 dibits)
    for(i = 0; i < 24; i++)
    {
      dibit = getDibit(opts, state);
      if(opts->inverted_dmr == 1)
      {
        dibit = (dibit ^ 2);
      }
      syncdata[i] = dibit;
      sync[i] = (dibit | 1) + 48;
    }
    sync[24] = 0;
    syncdata[24] = 0;

    // Next slot : Slot Type (second part) [10 bits / 5 dibits]
    for(i = 0; i < 5; i++)
    {
      dibit = getDibit(opts, state);
      if(opts->inverted_dmr == 1)
      {
        dibit = (dibit ^ 2);
      }
      SlotType[(i * 2) + 10] = (1 & (dibit >> 1)); // bit 1
      SlotType[(i * 2) + 10] = (1 & dibit);        // bit 0
    }

    // Next slot (98 last bits / 49 dibits)
    skipDibit (opts, state, 49);

    /* Check SYNC of 2nd slot */
    if((strcmp (sync, DMR_BS_DATA_SYNC) == 0) || (msMode == 1))
    {
      if(msMode) state->directmode = 1;
      else state->directmode = 0;
      if(state->currentslot == 0)
      {
        sprintf(state->slot2light, " slot2 ");
      }
      else
      {
        sprintf(state->slot1light, " slot1 ");
      }

      /* Check and correct the SlotType (apply Golay(20,8) FEC check) */
      if(Golay_20_8_decode((unsigned char *)SlotType)) SlotTypeOk = 1;
      else SlotTypeOk = 0;

      /* Reconstitute the burst type*/
      bursttype[0] = SlotType[4] + '0';
      bursttype[1] = SlotType[5] + '0';
      bursttype[2] = SlotType[6] + '0';
      bursttype[3] = SlotType[7] + '0';
      bursttype[4] = '\0';

      /* When PI Header or Voice Header detected on other slot, check if we
       * shall change the decoding slot */
      if ((strcmp (bursttype, "0000") == 0) || /* PI HEADER */
          (strcmp (bursttype, "0001") == 0))   /* VOICE Header */
      {
        /* Check if we need to decode this slot, if yes
         * go on to the processDMRdata() function */
        if(decode_next_slot && SlotTypeOk)
        {
          /* Interrupt the decoding of the current slot */
          interrupt_current_slot_decoding = 1;

        }
      }
    }
    else if(strcmp (sync, DMR_BS_VOICE_SYNC) == 0)
    {
      state->directmode = 0;
      if(state->currentslot == 0)
      {
        sprintf(state->slot2light, " SLOT2 ");
      }
      else
      {
        sprintf(state->slot1light, " SLOT1 ");
      }
      /* Check if we need to decode this slot, if yes
       * go on to the processDMRvoice() function */
      if(decode_next_slot)
      {
        interrupt_current_slot_decoding = 1;
        voice_sync_next_slot_detected = 1;
      }
    }
    else if((strcmp (sync, DMR_DIRECT_MODE_TS1_VOICE_SYNC) == 0) || (strcmp (sync, DMR_DIRECT_MODE_TS1_DATA_SYNC) == 0))
    {
      state->currentslot = 0;
      state->directmode = 1; /* Direct mode */
      sprintf(state->slot1light, " sLoT1 ");

      /* Check if we need to decode this slot, if yes
       * go on to the processDMRvoice() function */
      if(decode_next_slot)
      {
        /* Interrupt the decoding of the current slot */
        interrupt_current_slot_decoding = 1;
        voice_sync_next_slot_detected = 1;
      }
    }
    else if((strcmp (sync, DMR_DIRECT_MODE_TS2_VOICE_SYNC) == 0) || (strcmp (sync, DMR_DIRECT_MODE_TS2_DATA_SYNC) == 0))
    {
      state->currentslot = 1;
      state->directmode = 1; /* Direct mode */
      sprintf(state->slot1light, " sLoT2 ");

      /* Check if we need to decode this slot, if yes
       * go on to the processDMRvoice() function */
      if(decode_next_slot)
      {
        /* Interrupt the decoding of the current slot */
        interrupt_current_slot_decoding = 1;
        voice_sync_next_slot_detected = 1;
      }
    }

#ifdef DMR_DUMP
    k = 0;
    for(i = 0; i < 24; i++)
      {
        dibit = syncdata[i];
        syncbits[k] = (1 & (dibit >> 1)) + 48;        // bit 1
        k++;
        syncbits[k] = (1 & dibit) + 48;       // bit 0
        k++;
      }
    syncbits[48] = 0;
    fprintf(stderr, "%s ", syncbits);
#endif

    if(interrupt_current_slot_decoding)
    {
//      // 2nd half next slot (98 data bits after last "Slot Type" part = 49 dibits)
//      skipDibit (opts, state, 49);

      if(voice_sync_next_slot_detected)
      {
        /* Loop until the next Voice SYNC is detected */
        for(i = 0; i < 5; i++)
        {
          // CACH
          skipDibit (opts, state, 12);

          // first half current slot
          skipDibit (opts, state, 54);

          // SYNC current slot
          skipDibit (opts, state, 24);

          // second half current slot
          skipDibit (opts, state, 54);

          // CACH
          skipDibit (opts, state, 12);

          // first half next slot
          skipDibit (opts, state, 54);

          // SYNC next slot
          skipDibit (opts, state, 24);

          // second half next slot
          skipDibit (opts, state, 54);
        }
      } /* End if(voice_sync_next_slot_detected) */

      // CACH
      skipDibit (opts, state, 12);

      // first half current slot
      skipDibit (opts, state, 54);

      // SYNC current slot
      skipDibit (opts, state, 24);

      // second half current slot
      skipDibit (opts, state, 54);

      // CACH
      skipDibit (opts, state, 12);

      // first half next slot
      skipDibit (opts, state, 54);

      /* Exit the "for" loop */
      break;
    } /* End if(interrupt_current_slot_decoding) */

    if(j == 5)
    {
      // CACH
      skipDibit (opts, state, 12);

      // First half current slot (98 data bits = 49 dibits)
      skipDibit (opts, state, 49);

      // First half current slot (10 bits slot type = 5 dibits)
      //skipDibit (opts, state, 5); // TODO : Experimental : Do not skip the last 5 dibit to be sure to resynchronize the next SYNC correctly
      //NbOfDibitReadAndSkipped += 5;
      skipDibit (opts, state, 4); // Experimental : Skip only 4 dibit instead of 5 dibit (let 1 bit before the SYNC)

      /* At this step we are ready to decode the next valid SYNC */
    }
#ifdef USE_MBE
    /* Extract the 49 bits AMBE of the voice frame */
    if(state->currentslot == 0)
    {

      /* 3 x 49 bit of AMBE sample per voice frame => Slot 1buffer  here */
      for(i = 0; i < 3; i++)
      {
        state->TS1SuperFrame.TimeSlotAmbeVoiceFrame[j].errs1[i] = mbe_eccAmbe3600x2450C0(state->TS1SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[i]);
        state->TS1SuperFrame.TimeSlotAmbeVoiceFrame[j].errs2[i] = state->TS1SuperFrame.TimeSlotAmbeVoiceFrame[j].errs1[i];
        mbe_demodulateAmbe3600x2450Data(state->TS1SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[i]);
        state->TS1SuperFrame.TimeSlotAmbeVoiceFrame[j].errs2[i] += mbe_eccAmbe3600x2450Data(state->TS1SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[i],
                                                                                            &(state->TS1SuperFrame.TimeSlotAmbeVoiceFrame[j].AmbeBit[i][0]));
      }
    }
    else
    {
      /* 3 x 49 bit of AMBE sample per voice frame => Slot 2 buffer here */
      for(i = 0; i < 3; i++)
      {
        state->TS2SuperFrame.TimeSlotAmbeVoiceFrame[j].errs1[i] = mbe_eccAmbe3600x2450C0(state->TS2SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[i]);
        state->TS2SuperFrame.TimeSlotAmbeVoiceFrame[j].errs2[i] = state->TS2SuperFrame.TimeSlotAmbeVoiceFrame[j].errs1[i];
        mbe_demodulateAmbe3600x2450Data(state->TS2SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[i]);
        state->TS2SuperFrame.TimeSlotAmbeVoiceFrame[j].errs2[i] += mbe_eccAmbe3600x2450Data(state->TS2SuperFrame.TimeSlotDeinterleavedVoiceFrame[j].DeInterleavedVoiceSample[i],
                                                                                            &(state->TS2SuperFrame.TimeSlotAmbeVoiceFrame[j].AmbeBit[i][0]));
      }
    }
#endif
  }  /* End for(j = 0; j < 6; j++) */

  if(opts->errorbars == 1)
  {
    fprintf(stderr, "%s %s ", state->slot1light, state->slot2light);

    /* Print the color code */
    fprintf(stderr, "| Color Code=%02d ", (int)state->color_code);

    if(state->color_code_ok) fprintf(stderr, "(OK)      |");
    else fprintf(stderr, "(CRC ERR) |");

    fprintf(stderr, " VOICE         ");
  }

  /* Perform the SYNC DMR data embedded decoding */
  ProcessVoiceBurstSync(opts, state);

  /* Perform the DMR encryption decoding */
  ProcessDMREncryption(opts, state);

  /* Print DMR frame (if needed) */
  DMRVoiceFrameProcess(opts, state);

  if(opts->errorbars == 1)
  {
    fprintf(stderr, "\n");
  }
} /* End processDMRvoice() */
