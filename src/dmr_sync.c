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

//#define PRINT_PI_HEADER_BYTES
//#define PRINT_VOICE_LC_HEADER_BYTES
//#define PRINT_TERMINAISON_LC_BYTES
//#define PRINT_VOICE_BURST_BYTES
//#define PRINT_RATE_12_DATA_BYTES
//#define PRINT_RATE_34_DATA_BYTES


void ProcessDmrVoiceLcHeader(dsd_opts * opts, dsd_state * state, uint8_t info[196], uint8_t syncdata[48], uint8_t SlotType[20])
{
  uint32_t i, j, k;
  uint32_t CRCExtracted     = 0;
  uint32_t CRCComputed      = 0;
  uint32_t CRCCorrect       = 0;
  uint32_t IrrecoverableErrors = 0;
  uint8_t  DeInteleavedData[196];
  uint8_t  DmrDataBit[96];
  uint8_t  DmrDataByte[12];
  TimeSlotVoiceSuperFrame_t * TSVoiceSupFrame = NULL;
  uint8_t  R[3];
  uint8_t  BPTCReservedBits = 0;

  /* Remove warning compiler */
  UNUSED_VARIABLE(syncdata[0]);
  UNUSED_VARIABLE(SlotType[0]);
  UNUSED_VARIABLE(BPTCReservedBits);

  /* Check the current time slot */
  if(state->currentslot == 0)
  {
    TSVoiceSupFrame = &state->TS1SuperFrame;
  }
  else
  {
    TSVoiceSupFrame = &state->TS2SuperFrame;
  }

  CRCExtracted = 0;
  CRCComputed = 0;
  IrrecoverableErrors = 0;

  /* Deinterleave DMR data */
  BPTCDeInterleaveDMRData(info, DeInteleavedData);

  /* Extract the BPTC 196,96 DMR data */
  IrrecoverableErrors = BPTC_196x96_Extract_Data(DeInteleavedData, DmrDataBit, R);

  /* Fill the reserved bit (R(0)-R(2) of the BPTC(196,96) block) */
  BPTCReservedBits = (R[0] & 0x01) | ((R[1] << 1) & 0x02) | ((R[2] << 2) & 0x08);

  /* Convert the 96 bit of voice LC Header data into 12 bytes */
  k = 0;
  for(i = 0; i < 12; i++)
  {
    DmrDataByte[i] = 0;
    for(j = 0; j < 8; j++)
    {
      DmrDataByte[i] = DmrDataByte[i] << 1;
      DmrDataByte[i] = DmrDataByte[i] | (DmrDataBit[k] & 0x01);
      k++;
    }
  }

  /* Fill the CRC extracted (before Reed-Solomon (12,9) FEC correction) */
  CRCExtracted = 0;
  for(i = 0; i < 24; i++)
  {
    CRCExtracted = CRCExtracted << 1;
    CRCExtracted = CRCExtracted | (uint32_t)(DmrDataBit[i + 72] & 1);
  }

  /* Apply the CRC mask (see DMR standard B.3.12 Data Type CRC Mask) */
  CRCExtracted = CRCExtracted ^ 0x969696;

  /* Check/correct the full link control data and compute the Reed-Solomon (12,9) CRC */
  CRCCorrect = ComputeAndCorrectFullLinkControlCrc(DmrDataByte, &CRCComputed, 0x969696);

  /* Convert corrected 12 bytes into 96 bits */
  for(i = 0, j = 0; i < 12; i++, j+=8)
  {
    DmrDataBit[j + 0] = (DmrDataByte[i] >> 7) & 0x01;
    DmrDataBit[j + 1] = (DmrDataByte[i] >> 6) & 0x01;
    DmrDataBit[j + 2] = (DmrDataByte[i] >> 5) & 0x01;
    DmrDataBit[j + 3] = (DmrDataByte[i] >> 4) & 0x01;
    DmrDataBit[j + 4] = (DmrDataByte[i] >> 3) & 0x01;
    DmrDataBit[j + 5] = (DmrDataByte[i] >> 2) & 0x01;
    DmrDataBit[j + 6] = (DmrDataByte[i] >> 1) & 0x01;
    DmrDataBit[j + 7] = (DmrDataByte[i] >> 0) & 0x01;
  }

  /* Store the Protect Flag (PF) bit */
  TSVoiceSupFrame->FullLC.ProtectFlag = (unsigned int)(DmrDataBit[0]);

  /* Store the Reserved bit */
  TSVoiceSupFrame->FullLC.Reserved = (unsigned int)(DmrDataBit[1]);

  /* Store the Full Link Control Opcode (FLCO)  */
  TSVoiceSupFrame->FullLC.FullLinkControlOpcode = (unsigned int)ConvertBitIntoBytes(&DmrDataBit[2], 6);

  /* Store the Feature set ID (FID)  */
  TSVoiceSupFrame->FullLC.FeatureSetID = (unsigned int)ConvertBitIntoBytes(&DmrDataBit[8], 8);

  /* Store the Service Options  */
  TSVoiceSupFrame->FullLC.ServiceOptions = (unsigned int)ConvertBitIntoBytes(&DmrDataBit[16], 8);

  /* Store the Group address (Talk Group) */
  TSVoiceSupFrame->FullLC.GroupAddress = (unsigned int)ConvertBitIntoBytes(&DmrDataBit[24], 24);

  /* Store the Source address */
  TSVoiceSupFrame->FullLC.SourceAddress = (unsigned int)ConvertBitIntoBytes(&DmrDataBit[48], 24);

  if((IrrecoverableErrors == 0) && CRCCorrect)
  {
    /* CRC is correct so consider the Full LC data as correct/valid */
    TSVoiceSupFrame->FullLC.DataValidity = 1;
  }
  else
  {
    /* CRC checking error, so consider the Full LC data as invalid */
    TSVoiceSupFrame->FullLC.DataValidity = 0;
  }

  /* Print the destination ID (TG) and the source ID */
  fprintf(stderr, "| TG=%u  Src=%u ", TSVoiceSupFrame->FullLC.GroupAddress, TSVoiceSupFrame->FullLC.SourceAddress);
  fprintf(stderr, "FID=0x%02X ", TSVoiceSupFrame->FullLC.FeatureSetID);
  if(TSVoiceSupFrame->FullLC.ServiceOptions & 0x80) fprintf(stderr, "Emergency ");
  if(TSVoiceSupFrame->FullLC.ServiceOptions & 0x40)
  {
    /* By default select the basic privacy (BP), if the encryption mode is EP ARC4 or AES256
     * a PI Header will be sent with the encryption mode and DSD will upgrade automatically
     * the new encryption mode */
    opts->EncryptionMode = MODE_BASIC_PRIVACY;

    fprintf(stderr, "Encrypted ");
  }
  else
  {
    opts->EncryptionMode = MODE_UNENCRYPTED;
    fprintf(stderr, "Clear/Unencrypted ");
  }

  /* Check the "Reserved" bits */
  if(TSVoiceSupFrame->FullLC.ServiceOptions & 0x30)
  {
    /* Experimentally determined with DSD+, when the "Reserved" bit field
     * is equal to 0x2, this is a TXI call */
    if((TSVoiceSupFrame->FullLC.ServiceOptions & 0x30) == 0x20) fprintf(stderr, "TXI ");
    else fprintf(stderr, "Reserved=%d ", (TSVoiceSupFrame->FullLC.ServiceOptions & 0x30) >> 4);
  }
  if(TSVoiceSupFrame->FullLC.ServiceOptions & 0x08) fprintf(stderr, "Broadcast ");
  if(TSVoiceSupFrame->FullLC.ServiceOptions & 0x04) fprintf(stderr, "OVCM ");
  if(TSVoiceSupFrame->FullLC.ServiceOptions & 0x03)
  {
    if((TSVoiceSupFrame->FullLC.ServiceOptions & 0x03) == 0x01) fprintf(stderr, "Priority 1 ");
    else if((TSVoiceSupFrame->FullLC.ServiceOptions & 0x03) == 0x02) fprintf(stderr, "Priority 2 ");
    else if((TSVoiceSupFrame->FullLC.ServiceOptions & 0x03) == 0x03) fprintf(stderr, "Priority 3 ");
    else fprintf(stderr, "No Priority "); /* We should never go here */
  }
  fprintf(stderr, "Call ");

  if(TSVoiceSupFrame->FullLC.DataValidity) fprintf(stderr, "(OK) ");
  else if(IrrecoverableErrors == 0) fprintf(stderr, "RAS (FEC OK/CRC ERR)");
  else fprintf(stderr, "(FEC FAIL/CRC ERR)");

#ifdef PRINT_VOICE_LC_HEADER_BYTES
  fprintf(stderr, "\n");
  fprintf(stderr, "VOICE LC HEADER : ");
  for(i = 0; i < 12; i++)
  {
    fprintf(stderr, "0x%02X", DmrDataByte[i]);
    if(i != 11) fprintf(stderr, " - ");
  }

  fprintf(stderr, "\n");

  fprintf(stderr, "BPTC(196,96) Reserved bit R(0)-R(2) = 0x%02X\n", BPTCReservedBits);

  fprintf(stderr, "CRC extracted = 0x%04X - CRC computed = 0x%04X - ", CRCExtracted, CRCComputed);

  if((IrrecoverableErrors == 0) && CRCCorrect)
  {
    fprintf(stderr, "CRCs are equal + FEC OK !\n");
  }
  else if(IrrecoverableErrors == 0)
  {
    else fprintf(stderr, "FEC correctly corrected but CRCs are incorrect\n");
  }
  else
  {
    fprintf(stderr, "ERROR !!! CRCs are different and FEC Failed !\n");
  }

  fprintf(stderr, "Hamming Irrecoverable Errors = %u\n", IrrecoverableErrors);
#endif /* PRINT_VOICE_LC_HEADER_BYTES */
} /* End ProcessDmrVoiceLcHeader() */


void ProcessDmrTerminaisonLC(dsd_opts * opts, dsd_state * state, uint8_t info[196], uint8_t syncdata[48], uint8_t SlotType[20])
{
  uint32_t i, j, k;
  uint32_t CRCExtracted     = 0;
  uint32_t CRCComputed      = 0;
  uint32_t CRCCorrect       = 0;
  uint32_t IrrecoverableErrors = 0;
  uint8_t  DeInteleavedData[196];
  uint8_t  DmrDataBit[96];
  uint8_t  DmrDataByte[12];
  TimeSlotVoiceSuperFrame_t * TSVoiceSupFrame = NULL;
  uint8_t  R[3];
  uint8_t  BPTCReservedBits = 0;

  /* Remove warning compiler */
  UNUSED_VARIABLE(opts);
  UNUSED_VARIABLE(syncdata[0]);
  UNUSED_VARIABLE(SlotType[0]);
  UNUSED_VARIABLE(BPTCReservedBits);

  /* Check the current time slot */
  if(state->currentslot == 0)
  {
    TSVoiceSupFrame = &state->TS1SuperFrame;
  }
  else
  {
    TSVoiceSupFrame = &state->TS2SuperFrame;
  }

  CRCExtracted = 0;
  CRCComputed = 0;
  IrrecoverableErrors = 0;

  /* Deinterleave DMR data */
  BPTCDeInterleaveDMRData(info, DeInteleavedData);

  /* Extract the BPTC 196,96 DMR data */
  IrrecoverableErrors = BPTC_196x96_Extract_Data(DeInteleavedData, DmrDataBit, R);

  /* Fill the reserved bit (R(0)-R(2) of the BPTC(196,96) block) */
  BPTCReservedBits = (R[0] & 0x01) | ((R[1] << 1) & 0x02) | ((R[2] << 2) & 0x08);

  /* Convert the 96 bit of voice LC Header data into 12 bytes */
  k = 0;
  for(i = 0; i < 12; i++)
  {
    DmrDataByte[i] = 0;
    for(j = 0; j < 8; j++)
    {
      DmrDataByte[i] = DmrDataByte[i] << 1;
      DmrDataByte[i] = DmrDataByte[i] | (DmrDataBit[k] & 0x01);
      k++;
    }
  }

  /* Fill the CRC extracted (before Reed-Solomon (12,9) FEC correction) */
  CRCExtracted = 0;
  for(i = 0; i < 24; i++)
  {
    CRCExtracted = CRCExtracted << 1;
    CRCExtracted = CRCExtracted | (uint32_t)(DmrDataBit[i + 72] & 1);
  }

  /* Apply the CRC mask (see DMR standard B.3.12 Data Type CRC Mask) */
  CRCExtracted = CRCExtracted ^ 0x999999;

  /* Check/correct the full link control data and compute the Reed-Solomon (12,9) CRC */
  CRCCorrect = ComputeAndCorrectFullLinkControlCrc(DmrDataByte, &CRCComputed, 0x999999);

  /* Convert corrected 12 bytes into 96 bits */
  for(i = 0, j = 0; i < 12; i++, j+=8)
  {
    DmrDataBit[j + 0] = (DmrDataByte[i] >> 7) & 0x01;
    DmrDataBit[j + 1] = (DmrDataByte[i] >> 6) & 0x01;
    DmrDataBit[j + 2] = (DmrDataByte[i] >> 5) & 0x01;
    DmrDataBit[j + 3] = (DmrDataByte[i] >> 4) & 0x01;
    DmrDataBit[j + 4] = (DmrDataByte[i] >> 3) & 0x01;
    DmrDataBit[j + 5] = (DmrDataByte[i] >> 2) & 0x01;
    DmrDataBit[j + 6] = (DmrDataByte[i] >> 1) & 0x01;
    DmrDataBit[j + 7] = (DmrDataByte[i] >> 0) & 0x01;
  }

  /* Decode the content of the Link Control message */
  DmrFullLinkControlDecode(DmrDataBit, &TSVoiceSupFrame->FullLC, 0, 0);

//  /* Store the Protect Flag (PF) bit */
//  TSVoiceSupFrame->FullLC.ProtectFlag = (unsigned int)(DmrDataBit[0]);
//
//  /* Store the Reserved bit */
//  TSVoiceSupFrame->FullLC.Reserved = (unsigned int)(DmrDataBit[1]);
//
//  /* Store the Full Link Control Opcode (FLCO)  */
//  TSVoiceSupFrame->FullLC.FullLinkControlOpcode = (unsigned int)ConvertBitIntoBytes(&DmrDataBit[2], 6);
//
//  /* Store the Feature set ID (FID)  */
//  TSVoiceSupFrame->FullLC.FeatureSetID = (unsigned int)ConvertBitIntoBytes(&DmrDataBit[8], 8);
//
//  /* Store the Service Options  */
//  TSVoiceSupFrame->FullLC.ServiceOptions = (unsigned int)ConvertBitIntoBytes(&DmrDataBit[16], 8);
//
//  /* Store the Group address (Talk Group) */
//  TSVoiceSupFrame->FullLC.GroupAddress = (unsigned int)ConvertBitIntoBytes(&DmrDataBit[24], 24);
//
//  /* Store the Source address */
//  TSVoiceSupFrame->FullLC.SourceAddress = (unsigned int)ConvertBitIntoBytes(&DmrDataBit[48], 24);

  if((IrrecoverableErrors == 0))// && CRCCorrect)
  {
    /* CRC is correct so consider the Full LC data as correct/valid */
    TSVoiceSupFrame->FullLC.DataValidity = 1;
  }
  else
  {
    /* CRC checking error, so consider the Full LC data as invalid */
    TSVoiceSupFrame->FullLC.DataValidity = 0;
  }

  /* Print the destination ID (TG) and the source ID */
  //fprintf(stderr, "| TG=%u  Src=%u ", TSVoiceSupFrame->FullLC.GroupAddress, TSVoiceSupFrame->FullLC.SourceAddress);
  fprintf(stderr, "FID=0x%02X ", TSVoiceSupFrame->FullLC.FeatureSetID);

  if((IrrecoverableErrors == 0) && CRCCorrect)
  {
    fprintf(stderr, "(OK) ");
  }
  else if(IrrecoverableErrors == 0)
  {
    fprintf(stderr, "RAS (FEC OK/CRC ERR)");
  }
  else fprintf(stderr, "(FEC FAIL/CRC ERR)");

#ifdef PRINT_TERMINAISON_LC_BYTES
  fprintf(stderr, "\n");
  fprintf(stderr, "TERMINAISON LINK CONTROL (TLC) : ");
  for(i = 0; i < 12; i++)
  {
    fprintf(stderr, "0x%02X", DmrDataByte[i]);
    if(i != 11) fprintf(stderr, " - ");
  }

  fprintf(stderr, "\n");

  fprintf(stderr, "BPTC(196,96) Reserved bit R(0)-R(2) = 0x%02X\n", BPTCReservedBits);

  fprintf(stderr, "CRC extracted = 0x%04X - CRC computed = 0x%04X - ", CRCExtracted, CRCComputed);

  if((IrrecoverableErrors == 0) && CRCCorrect)
  {
    fprintf(stderr, "CRCs are equal + FEC OK !\n");
  }
  else if(IrrecoverableErrors == 0)
  {
    else fprintf(stderr, "FEC correctly corrected but CRCs are incorrect\n");
  }
  else
  {
    fprintf(stderr, "ERROR !!! CRCs are different and FEC Failed !\n");
  }

  fprintf(stderr, "Hamming Irrecoverable Errors = %u\n", IrrecoverableErrors);
#endif /* PRINT_TERMINAISON_LC_BYTES */
} /* End ProcessDmrTerminaisonLC() */


/* Extract the Link Control (LC) embedded in the SYNC
 * of a DMR voice superframe */
void ProcessVoiceBurstSync(dsd_opts * opts, dsd_state * state)
{
  uint32_t i, j, k;
  uint32_t Burst;
  uint8_t  BptcDataMatrix[8][16];
  uint8_t  LC_DataBit[77 + 19] = {0}; /* 72 bits + 5 bits CRC + 19 padding bits to remove compiler warning when calling the "DmrFullLinkControlDecode" function */
  uint8_t  LC_DataBytes[10];
  TimeSlotVoiceSuperFrame_t * TSVoiceSupFrame = NULL;
  uint32_t IrrecoverableErrors;
  uint8_t  CRCExtracted;
  uint8_t  CRCComputed;
  uint32_t CRCCorrect = 0;

  /* Remove warning compiler */
  UNUSED_VARIABLE(opts);

  /* Check the current time slot */
  if(state->currentslot == 0)
  {
    TSVoiceSupFrame = &state->TS1SuperFrame;
  }
  else
  {
    TSVoiceSupFrame = &state->TS2SuperFrame;
  }

  /* First step : Reconstitute the BPTC 16x8 matrix */
  Burst = 1; /* Burst B to E contains embedded signaling data */
  k = 0;
  for(i = 0; i < 16; i++)
  {
    for(j = 0; j < 8; j++)
    {
      /* Only the LSBit of the byte is stored */
      BptcDataMatrix[j][i] = TSVoiceSupFrame->TimeSlotRawVoiceFrame[Burst].Sync[k + 8];
      k++;

      /* Go on to the next burst once 32 bit
       * of the SNYC have been stored */
      if(k >= 32)
      {
        k = 0;
        Burst++;
      }
    } /* End for(j = 0; j < 8; j++) */
  } /* End for(i = 0; i < 16; i++) */

  /* Extract the 72 LC bit (+ 5 CRC bit) of the matrix */
  IrrecoverableErrors = BPTC_128x77_Extract_Data(BptcDataMatrix, LC_DataBit);

  /* Convert the 77 bit of voice LC Header data into 9 bytes */
  k = 0;
  for(i = 0; i < 10; i++)
  {
    LC_DataBytes[i] = 0;
    for(j = 0; j < 8; j++)
    {
      LC_DataBytes[i] = LC_DataBytes[i] << 1;
      LC_DataBytes[i] = LC_DataBytes[i] | (LC_DataBit[k] & 0x01);
      k++;
      if(k >= 76) break;
    }
  }

  /* Reconstitute the 5 bit CRC */
  CRCExtracted  = (LC_DataBit[72] & 1) << 4;
  CRCExtracted |= (LC_DataBit[73] & 1) << 3;
  CRCExtracted |= (LC_DataBit[74] & 1) << 2;
  CRCExtracted |= (LC_DataBit[75] & 1) << 1;
  CRCExtracted |= (LC_DataBit[76] & 1) << 0;

  /* Compute the 5 bit CRC */
  CRCComputed = ComputeCrc5Bit(LC_DataBit);

  if(CRCExtracted == CRCComputed) CRCCorrect = 1;
  else CRCCorrect = 0;


  /* Decode the content of the Link Control message */
  DmrFullLinkControlDecode(LC_DataBit, &TSVoiceSupFrame->FullLC, 1, 0);

//  /* Store the Protect Flag (PF) bit */
//  TSVoiceSupFrame->FullLC.ProtectFlag = (unsigned int)(LC_DataBit[0]);
//
//  /* Store the Reserved bit */
//  TSVoiceSupFrame->FullLC.Reserved = (unsigned int)(LC_DataBit[1]);
//
//  /* Store the Full Link Control Opcode (FLCO)  */
//  TSVoiceSupFrame->FullLC.FullLinkControlOpcode = (unsigned int)ConvertBitIntoBytes(&LC_DataBit[2], 6);
//
//  /* Store the Feature set ID (FID)  */
//  TSVoiceSupFrame->FullLC.FeatureSetID = (unsigned int)ConvertBitIntoBytes(&LC_DataBit[8], 8);
//
//  /* Store the Service Options  */
//  TSVoiceSupFrame->FullLC.ServiceOptions = (unsigned int)ConvertBitIntoBytes(&LC_DataBit[16], 8);
//
//  /* Store the Group address (Talk Group) */
//  TSVoiceSupFrame->FullLC.GroupAddress = (unsigned int)ConvertBitIntoBytes(&LC_DataBit[24], 24);
//
//  /* Store the Source address */
//  TSVoiceSupFrame->FullLC.SourceAddress = (unsigned int)ConvertBitIntoBytes(&LC_DataBit[48], 24);

  /* Check the CRC values */
  if((IrrecoverableErrors == 0))// && CRCCorrect)
  {
    /* Data ara correct */
    //fprintf(stderr, "\nLink Control (LC) Data CRCs are correct !!! Number of error = %u\n", NbOfError);

    /* CRC is correct so consider the Full LC data as correct/valid */
    TSVoiceSupFrame->FullLC.DataValidity = 1;
  }
  else
  {
    /* CRC checking error, so consider the Full LC data as invalid */
    TSVoiceSupFrame->FullLC.DataValidity = 0;
  }

  /* Print the destination ID (TG) and the source ID */
  //fprintf(stderr, "| TG=%u  Src=%u ", TSVoiceSupFrame->FullLC.GroupAddress, TSVoiceSupFrame->FullLC.SourceAddress);
  fprintf(stderr, "FID=0x%02X ", TSVoiceSupFrame->FullLC.FeatureSetID);

  if((IrrecoverableErrors == 0) && CRCCorrect)
  {
    fprintf(stderr, "(OK)");
  }
  else if(IrrecoverableErrors == 0)
  {
    fprintf(stderr, "RAS (FEC OK/CRC ERR)");
  }
  else fprintf(stderr, "(FEC FAIL/CRC ERR)");

#ifdef PRINT_VOICE_BURST_BYTES
  fprintf(stderr, "\n");
  fprintf(stderr, "VOICE BURST BYTES : ");
  for(i = 0; i < 10; i++)
  {
    fprintf(stderr, "0x%02X", LC_DataBytes[i]);
    if(i != 9) fprintf(stderr, " - ");
  }

  fprintf(stderr, "\n");

  fprintf(stderr, "CRC extracted = 0x%04X - CRC computed = 0x%04X - ", CRCExtracted, CRCComputed);

  if((IrrecoverableErrors == 0) && CRCCorrect)
  {
    fprintf(stderr, "CRCs are equal + FEC OK !\n");
  }
  else if(IrrecoverableErrors == 0)
  {
    fprintf(stderr, "FEC correctly corrected but CRCs are incorrect\n");
  }
  else
  {
    fprintf(stderr, "ERROR !!! CRCs are different and FEC Failed !\n");
  }

  fprintf(stderr, "Hamming Irrecoverable Errors = %u\n", IrrecoverableErrors);
#endif /* PRINT_VOICE_BURST_BYTES */

} /* End ProcessVoiceBurstSync() */


void ProcessDmrCSBK(dsd_opts * opts, dsd_state * state, uint8_t info[196], uint8_t syncdata[48], uint8_t SlotType[20])
{
  uint32_t i, j, k;
  uint16_t CRCExtracted     = 0;
  uint16_t CRCComputed      = 0;
  uint32_t CRCCorrect       = 0;
  uint32_t IrrecoverableErrors = 0;
  uint8_t  FID              = 0;
  uint8_t  DeInteleavedData[196];
  uint8_t  DmrDataBit[96];
  uint8_t  DmrDataByte[12];
  uint8_t  R[3];
  uint8_t  BPTCReservedBits = 0;
  TimeSlotVoiceSuperFrame_t * TSVoiceSupFrame = NULL;

  /* Remove warning compiler */
  UNUSED_VARIABLE(opts);
  UNUSED_VARIABLE(state);
  UNUSED_VARIABLE(syncdata[0]);
  UNUSED_VARIABLE(SlotType[0]);
  UNUSED_VARIABLE(BPTCReservedBits);
  UNUSED_VARIABLE(FID);

  CRCExtracted = 0;
  CRCComputed = 0;
  IrrecoverableErrors = 0;

  /* Check the current time slot */
  if(state->currentslot == 0)
  {
    TSVoiceSupFrame = &state->TS1SuperFrame;
  }
  else
  {
    TSVoiceSupFrame = &state->TS2SuperFrame;
  }

  /* Deinterleave DMR data */
  BPTCDeInterleaveDMRData(info, DeInteleavedData);

  /* Extract the BPTC 196,96 DMR data */
  IrrecoverableErrors = BPTC_196x96_Extract_Data(DeInteleavedData, DmrDataBit, R);

  /* Fill the reserved bit (R(0)-R(2) of the BPTC(196,96) block) */
  BPTCReservedBits = (R[0] & 0x01) | ((R[1] << 1) & 0x02) | ((R[2] << 2) & 0x08);

  /* Fill the CRC extracted */
  CRCExtracted = 0;
  for(i = 0; i < 16; i++)
  {
    CRCExtracted = CRCExtracted << 1;
    CRCExtracted = CRCExtracted | (uint32_t)(DmrDataBit[i + 80] & 1);
  }

  /* Apply the CRC mask (see DMR standard B.3.12 Data Type CRC Mask) */
  CRCExtracted = CRCExtracted ^ 0xA5A5;

  /* Compute the CRC based on the received data */
  CRCComputed = ComputeCrcCCITT(DmrDataBit);

  /* Check the CRCs */
  if(CRCExtracted == CRCComputed)
  {
    /* CRCs are equal => Good ! */
    CRCCorrect = 1;
  }
  else
  {
    /* CRCs different => Error */
    CRCCorrect = 0;
  }

  /* Convert the 96 bits of CSBK data into 12 bytes */
  k = 0;
  for(i = 0; i < 12; i++)
  {
    DmrDataByte[i] = 0;
    for(j = 0; j < 8; j++)
    {
      DmrDataByte[i] = DmrDataByte[i] << 1;
      DmrDataByte[i] = DmrDataByte[i] | (DmrDataBit[k] & 0x01);
      k++;
    }
  }

  /* Fill the feature Set ID */
  FID = DmrDataByte[1] & 0xFF;

  /* Decode the content of the Link Control message */
  DmrFullLinkControlDecode(DmrDataBit, &TSVoiceSupFrame->FullLC, 0, 1);

  fprintf(stderr, "FID=0x%02X ", FID);
  if((IrrecoverableErrors == 0) && CRCCorrect)
  {
    fprintf(stderr, "(OK)");
  }
  else if(IrrecoverableErrors == 0)
  {
    fprintf(stderr, "RAS (FEC OK/CRC ERR)");
  }
  else fprintf(stderr, "(FEC FAIL/CRC ERR)");

} /* End ProcessDmrCSBK() */


void ProcessDmrDataHeader(dsd_opts * opts, dsd_state * state, uint8_t info[196], uint8_t syncdata[48], uint8_t SlotType[20])
{
  uint32_t i, j, k;
  uint16_t CRCExtracted     = 0;
  uint16_t CRCComputed      = 0;
  uint32_t CRCCorrect       = 0;
  uint32_t IrrecoverableErrors = 0;
  uint8_t  DeInteleavedData[196];
  uint8_t  DmrDataBit[96];
  uint8_t  DmrDataByte[12];
  uint8_t  R[3];
  uint8_t  BPTCReservedBits = 0;
  TimeSlotVoiceSuperFrame_t * TSVoiceSupFrame = NULL;

  /* Remove warning compiler */
  UNUSED_VARIABLE(opts);
  UNUSED_VARIABLE(state);
  UNUSED_VARIABLE(syncdata[0]);
  UNUSED_VARIABLE(SlotType[0]);
  UNUSED_VARIABLE(BPTCReservedBits);

  CRCExtracted = 0;
  CRCComputed = 0;
  IrrecoverableErrors = 0;

  /* Check the current time slot */
  if(state->currentslot == 0)
  {
    TSVoiceSupFrame = &state->TS1SuperFrame;
  }
  else
  {
    TSVoiceSupFrame = &state->TS2SuperFrame;
  }

  /* Clear all previous data received before */
  memset(&TSVoiceSupFrame->Data, 0, sizeof(DMRDataPDU_t));

  /* Initialize Block ID */
  TSVoiceSupFrame->Data.Rate12NbOfReceivedBlock = 0;
  TSVoiceSupFrame->Data.Rate34NbOfReceivedBlock = 0;

  /* Deinterleave DMR data */
  BPTCDeInterleaveDMRData(info, DeInteleavedData);

  /* Extract the BPTC 196,96 DMR data */
  IrrecoverableErrors = BPTC_196x96_Extract_Data(DeInteleavedData, DmrDataBit, R);

  /* Fill the reserved bit (R(0)-R(2) of the BPTC(196,96) block) */
  BPTCReservedBits = (R[0] & 0x01) | ((R[1] << 1) & 0x02) | ((R[2] << 2) & 0x08);

  /* Fill the CRC extracted */
  CRCExtracted = 0;
  for(i = 0; i < 16; i++)
  {
    CRCExtracted = CRCExtracted << 1;
    CRCExtracted = CRCExtracted | (uint32_t)(DmrDataBit[i + 80] & 1);
  }

  /* Apply the CRC mask (see DMR standard B.3.12 Data Type CRC Mask) */
  CRCExtracted = CRCExtracted ^ 0xCCCC;

  /* Compute the CRC based on the received data */
  CRCComputed = ComputeCrcCCITT(DmrDataBit);

  /* Check the CRCs */
  if(CRCExtracted == CRCComputed)
  {
    /* CRCs are equal => Good ! */
    CRCCorrect = 1;
  }
  else
  {
    /* CRCs different => Error */
    CRCCorrect = 0;
  }

  /* Convert the 96 bits of Data header into 12 bytes */
  k = 0;
  for(i = 0; i < 12; i++)
  {
    DmrDataByte[i] = 0;
    for(j = 0; j < 8; j++)
    {
      DmrDataByte[i] = DmrDataByte[i] << 1;
      DmrDataByte[i] = DmrDataByte[i] | (DmrDataBit[k] & 0x01);
      k++;
    }
  }

  /* Decode the content of the data header message */
  DmrDataHeaderDecode(DmrDataBit, &TSVoiceSupFrame->Data);

  fprintf(stderr, "| Data=0x%02X", DmrDataByte[0] & 0xFF);
  for(i = 1; i < 12; i++)
  {
    fprintf(stderr, "-0x%02X", DmrDataByte[i] & 0xFF);
  }
  fprintf(stderr, "  ");

  if((IrrecoverableErrors == 0) && CRCCorrect)
  {
    fprintf(stderr, "(OK)");
  }
  else if(IrrecoverableErrors == 0)
  {
    fprintf(stderr, "RAS (FEC OK/CRC ERR)");
  }
  else fprintf(stderr, "(FEC FAIL/CRC ERR)");

} /* End ProcessDmrDataHeader() */


void ProcessDmr12Data(dsd_opts * opts, dsd_state * state, uint8_t info[196], uint8_t syncdata[48], uint8_t SlotType[20])
{
  uint32_t i, j, k;
  uint32_t CRCExtracted     = 0;
  uint32_t CRCComputed      = 0;
  uint32_t CRCCorrect       = 0;
  uint32_t IrrecoverableErrors = 0;
  uint8_t  DeInteleavedData[196];
  uint8_t  DmrDataBit[96];
  uint8_t  DmrDataBitBigEndian[96];
  uint8_t  DmrDataByte[12];
  uint16_t DmrDataWordBigEndian[6];
  TimeSlotVoiceSuperFrame_t * TSVoiceSupFrame = NULL;
  uint8_t  R[3];
  uint8_t  BPTCReservedBits = 0;

  /* Remove warning compiler */
  UNUSED_VARIABLE(opts);
  UNUSED_VARIABLE(state);
  UNUSED_VARIABLE(syncdata[0]);
  UNUSED_VARIABLE(SlotType[0]);
  UNUSED_VARIABLE(BPTCReservedBits);

  /* Check the current time slot */
  if(state->currentslot == 0)
  {
    TSVoiceSupFrame = &state->TS1SuperFrame;
  }
  else
  {
    TSVoiceSupFrame = &state->TS2SuperFrame;
  }

  /* Deinterleave DMR data */
  BPTCDeInterleaveDMRData(info, DeInteleavedData);

  /* Extract the BPTC 196,96 DMR data */
  IrrecoverableErrors = BPTC_196x96_Extract_Data(DeInteleavedData, DmrDataBit, R);

  /* Fill the reserved bit (R(0)-R(2) of the BPTC(196,96) block) */
  BPTCReservedBits = (R[0] & 0x01) | ((R[1] << 1) & 0x02) | ((R[2] << 2) & 0x08);

  /* Convert 96 bits to 12 bytes */
  k = 0;
  for(i = 0; i < 12; i++)
  {
    DmrDataByte[i] = 0;
    for(j = 0; j < 8; j++)
    {
      DmrDataByte[i] = DmrDataByte[i] << 1;
      DmrDataByte[i] = DmrDataByte[i] | (DmrDataBit[k] & 0x01);
      k++;
    }
  }

  /* Convert bytes from little endian format to 16 bits word big endian format (used to compute the CRC-32) */
  for(i = 0; i < 6; i++)
  {
    DmrDataWordBigEndian[i] = ((DmrDataByte[(2 * i) + 1] & 0xFF) << 8) | (DmrDataByte[2 * i] & 0xFF);
    TSVoiceSupFrame->Data.Rate12DataWordBigEndian[(TSVoiceSupFrame->Data.Rate12NbOfReceivedBlock * 6) + i] = DmrDataWordBigEndian[i] & 0xFFFF;

    /* Decompose the 16 bit word big endian format into bit */
    DmrDataBitBigEndian[(i * 16) + 0]  = (DmrDataWordBigEndian[i] >> 15) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 1]  = (DmrDataWordBigEndian[i] >> 14) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 2]  = (DmrDataWordBigEndian[i] >> 13) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 3]  = (DmrDataWordBigEndian[i] >> 12) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 4]  = (DmrDataWordBigEndian[i] >> 11) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 5]  = (DmrDataWordBigEndian[i] >> 10) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 6]  = (DmrDataWordBigEndian[i] >>  9) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 7]  = (DmrDataWordBigEndian[i] >>  8) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 8]  = (DmrDataWordBigEndian[i] >>  7) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 9]  = (DmrDataWordBigEndian[i] >>  6) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 10] = (DmrDataWordBigEndian[i] >>  5) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 11] = (DmrDataWordBigEndian[i] >>  4) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 12] = (DmrDataWordBigEndian[i] >>  3) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 13] = (DmrDataWordBigEndian[i] >>  2) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 14] = (DmrDataWordBigEndian[i] >>  1) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 15] = (DmrDataWordBigEndian[i] >>  0) & 0x01;
  }

  /* Copy the frame content into the rate 1/2 buffers */
  for(i = 0; i < 96; i++)
  {
    if(i >= 16) TSVoiceSupFrame->Data.Rate12DataBitBigEndianConfirmed[(TSVoiceSupFrame->Data.Rate12NbOfReceivedBlock * 80) + i - 16] = DmrDataBitBigEndian[i] & 0x01;
    TSVoiceSupFrame->Data.Rate12DataBitBigEndianUnconfirmed[(TSVoiceSupFrame->Data.Rate12NbOfReceivedBlock * 96) + i] = DmrDataBitBigEndian[i] & 0x01;
    TSVoiceSupFrame->Data.Rate12DataBit[(TSVoiceSupFrame->Data.Rate12NbOfReceivedBlock * 96) + i] = DmrDataBit[i] & 0x01;
  }

  /* Increment the number of data block received */
  TSVoiceSupFrame->Data.Rate12NbOfReceivedBlock++;

  fprintf(stderr, "| Data=0x%02X", DmrDataByte[0] & 0xFF);
  for(i = 1; i < 12; i++)
  {
    fprintf(stderr, "-0x%02X", DmrDataByte[i] & 0xFF);
  }
  fprintf(stderr, "  ");

  /* Extract and compute CRC only at the end of the transmission */
  if(TSVoiceSupFrame->Data.Rate12NbOfReceivedBlock >= TSVoiceSupFrame->Data.BlocksToFollow)
  {
    /* Fill the CRC extracted (stored into little endian format) */
    CRCExtracted = ((DmrDataByte[11] & 0xFF) << 24) | ((DmrDataByte[10] & 0xFF) << 16) | ((DmrDataByte[9] & 0xFF) << 8) | (DmrDataByte[8] & 0xFF);

    /* CRC is different if data are confirmed or unconfirmed */
    if((TSVoiceSupFrame->Data.DataPacketFormat & 0b1111) == 0b0011)
    {
      /* Confirmed data */
      /* Compute the CRC */
      CRCComputed = ComputeCrc32Bit((uint8_t *)TSVoiceSupFrame->Data.Rate12DataBitBigEndianConfirmed, (TSVoiceSupFrame->Data.Rate12NbOfReceivedBlock * 80) - 32);
    }
    else if((TSVoiceSupFrame->Data.DataPacketFormat & 0b1111) == 0b0010)
    {
      /* Unconfirmed data */
      /* Compute the CRC */
      CRCComputed = ComputeCrc32Bit((uint8_t *)TSVoiceSupFrame->Data.Rate12DataBitBigEndianUnconfirmed, (TSVoiceSupFrame->Data.Rate12NbOfReceivedBlock * 96) - 32);
    }
    else
    {
      /* Default : Consider data as Unconfirmed data */
      /* Compute the CRC */
      CRCComputed = ComputeCrc32Bit((uint8_t *)TSVoiceSupFrame->Data.Rate12DataBitBigEndianUnconfirmed, (TSVoiceSupFrame->Data.Rate12NbOfReceivedBlock * 96) - 32);
    }

    /* Check the CRCs */
    if(CRCExtracted == CRCComputed)
    {
      /* CRCs are equal => Good ! */
      CRCCorrect = 1;
    }
    else
    {
      /* CRCs different => Error */
      CRCCorrect = 0;
    }

    if((IrrecoverableErrors == 0) && CRCCorrect)
    {
      fprintf(stderr, "(OK)");
    }
    else if(IrrecoverableErrors == 0)
    {
      fprintf(stderr, "RAS (FEC OK/CRC ERR)");
    }
    else fprintf(stderr, "(FEC FAIL/CRC ERR)");
  }
  else
  {
    if(IrrecoverableErrors == 0) fprintf(stderr, "(OK)");
    else fprintf(stderr, "(FEC FAIL/CRC ERR)");
  }

#ifdef PRINT_RATE_12_DATA_BYTES
  fprintf(stderr, "\n");
  fprintf(stderr, "RATE 1/2 DATA : ");
  for(i = 0; i < 12; i++)
  {
    fprintf(stderr, "0x%02X", DmrDataByte[i]);
    if(i != 11) fprintf(stderr, " - ");
  }

  fprintf(stderr, "\n");

  fprintf(stderr, "CRC extracted  = 0x%08X - CRC computed  = 0x%08X - ", CRCExtracted, CRCComputed);

  if((IrrecoverableErrors == 0) && CRCCorrect)
  {
    fprintf(stderr, "CRCs are equal + FEC OK !\n");
  }
  else if(IrrecoverableErrors == 0)
  {
    fprintf(stderr, "FEC correctly corrected but CRCs are incorrect\n");
  }
  else
  {
    fprintf(stderr, "ERROR !!! CRCs are different and FEC Failed !\n");
  }

  fprintf(stderr, "Hamming Irrecoverable Errors = %u\n", IrrecoverableErrors);
#endif /* PRINT_RATE_12_DATA_BYTES */
} /* End ProcessDmr12Data() */


void ProcessDmr34Data(dsd_opts * opts, dsd_state * state, uint8_t tdibits[98], uint8_t syncdata[48], uint8_t SlotType[20])
{
  uint32_t i, j;
  uint32_t CRCExtracted     = 0;
  uint32_t CRCComputed      = 0;
  uint32_t CRCCorrect       = 0;
  uint32_t IrrecoverableErrors = 0;
  uint8_t  DmrDataBit[144];
  uint8_t  DmrDataBitBigEndian[144];
  uint8_t  DmrDataByte[18];
  uint16_t DmrDataWordBigEndian[9];
  TimeSlotVoiceSuperFrame_t * TSVoiceSupFrame = NULL;

  /* Remove warning compiler */
  UNUSED_VARIABLE(opts);
  UNUSED_VARIABLE(state);
  UNUSED_VARIABLE(tdibits);
  UNUSED_VARIABLE(syncdata[0]);
  UNUSED_VARIABLE(SlotType[0]);

  /* Check the current time slot */
  if(state->currentslot == 0)
  {
    TSVoiceSupFrame = &state->TS1SuperFrame;
  }
  else
  {
    TSVoiceSupFrame = &state->TS2SuperFrame;
  }

  if(CDMRTrellis_decode(tdibits, DmrDataBit) == true) IrrecoverableErrors = 0;

  for(i = 0, j = 0; i < 18; i++, j+=8)
  {
    DmrDataByte[i]  = (DmrDataBit[j + 0] & 1) << 7;
    DmrDataByte[i] |= (DmrDataBit[j + 1] & 1) << 6;
    DmrDataByte[i] |= (DmrDataBit[j + 2] & 1) << 5;
    DmrDataByte[i] |= (DmrDataBit[j + 3] & 1) << 4;
    DmrDataByte[i] |= (DmrDataBit[j + 4] & 1) << 3;
    DmrDataByte[i] |= (DmrDataBit[j + 5] & 1) << 2;
    DmrDataByte[i] |= (DmrDataBit[j + 6] & 1) << 1;
    DmrDataByte[i] |= (DmrDataBit[j + 7] & 1) << 0;
  }

  /* Convert bytes from little endian format to 16 bits word big endian format (used to compute the CRC-32) */
  for(i = 0; i < 9; i++)
  {
    DmrDataWordBigEndian[i] = ((DmrDataByte[(2 * i) + 1] & 0xFF) << 8) | (DmrDataByte[2 * i] & 0xFF);
    TSVoiceSupFrame->Data.Rate34DataWordBigEndian[(TSVoiceSupFrame->Data.Rate34NbOfReceivedBlock * 9) + i] = DmrDataWordBigEndian[i] & 0xFFFF;

    /* Decompose the 16 bit word big endian format into bit */
    DmrDataBitBigEndian[(i * 16) + 0]  = (DmrDataWordBigEndian[i] >> 15) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 1]  = (DmrDataWordBigEndian[i] >> 14) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 2]  = (DmrDataWordBigEndian[i] >> 13) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 3]  = (DmrDataWordBigEndian[i] >> 12) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 4]  = (DmrDataWordBigEndian[i] >> 11) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 5]  = (DmrDataWordBigEndian[i] >> 10) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 6]  = (DmrDataWordBigEndian[i] >>  9) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 7]  = (DmrDataWordBigEndian[i] >>  8) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 8]  = (DmrDataWordBigEndian[i] >>  7) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 9]  = (DmrDataWordBigEndian[i] >>  6) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 10] = (DmrDataWordBigEndian[i] >>  5) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 11] = (DmrDataWordBigEndian[i] >>  4) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 12] = (DmrDataWordBigEndian[i] >>  3) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 13] = (DmrDataWordBigEndian[i] >>  2) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 14] = (DmrDataWordBigEndian[i] >>  1) & 0x01;
    DmrDataBitBigEndian[(i * 16) + 15] = (DmrDataWordBigEndian[i] >>  0) & 0x01;
  }

  /* Copy the frame content into the rate 1/2 buffers */
  for(i = 0; i < 144; i++)
  {
    if(i >= 16) TSVoiceSupFrame->Data.Rate34DataBitBigEndianConfirmed[(TSVoiceSupFrame->Data.Rate34NbOfReceivedBlock * 128) + i - 16] = DmrDataBitBigEndian[i] & 0x01;
    TSVoiceSupFrame->Data.Rate34DataBitBigEndianUnconfirmed[(TSVoiceSupFrame->Data.Rate34NbOfReceivedBlock * 144) + i] = DmrDataBitBigEndian[i] & 0x01;
    TSVoiceSupFrame->Data.Rate34DataBit[(TSVoiceSupFrame->Data.Rate34NbOfReceivedBlock * 144) + i] = DmrDataBit[i] & 0x01;
  }

  /* Increment the number of data block received */
  TSVoiceSupFrame->Data.Rate34NbOfReceivedBlock++;

  fprintf(stderr, "| Data=0x%02X", DmrDataByte[0] & 0xFF);
  for(i = 1; i < 18; i++)
  {
    fprintf(stderr, "-0x%02X", DmrDataByte[i] & 0xFF);
  }
  fprintf(stderr, "  ");

  /* Extract and compute CRC only at the end of the transmission */
  if(TSVoiceSupFrame->Data.Rate34NbOfReceivedBlock >= TSVoiceSupFrame->Data.BlocksToFollow)
  {
    /* Fill the CRC extracted (stored into little endian format) */
    CRCExtracted = ((DmrDataByte[17] & 0xFF) << 24) | ((DmrDataByte[16] & 0xFF) << 16) | ((DmrDataByte[15] & 0xFF) << 8) | (DmrDataByte[14] & 0xFF);

    /* CRC is different if data are confirmed or unconfirmed */
    if((TSVoiceSupFrame->Data.DataPacketFormat & 0b1111) == 0b0011)
    {
      /* Confirmed data */
      /* Compute the CRC */
      CRCComputed = ComputeCrc32Bit((uint8_t *)TSVoiceSupFrame->Data.Rate34DataBitBigEndianConfirmed, (TSVoiceSupFrame->Data.Rate34NbOfReceivedBlock * 128) - 32);
    }
    else if((TSVoiceSupFrame->Data.DataPacketFormat & 0b1111) == 0b0010)
    {
      /* Unconfirmed data */
      /* Compute the CRC */
      CRCComputed = ComputeCrc32Bit((uint8_t *)TSVoiceSupFrame->Data.Rate34DataBitBigEndianUnconfirmed, (TSVoiceSupFrame->Data.Rate34NbOfReceivedBlock * 144) - 32);
    }
    else
    {
      /* Default : Consider data as Unconfirmed data */
      /* Compute the CRC */
      CRCComputed = ComputeCrc32Bit((uint8_t *)TSVoiceSupFrame->Data.Rate34DataBitBigEndianUnconfirmed, (TSVoiceSupFrame->Data.Rate34NbOfReceivedBlock * 144) - 32);
    }

    /* Check the CRCs */
    if(CRCExtracted == CRCComputed)
    {
      /* CRCs are equal => Good ! */
      CRCCorrect = 1;
    }
    else
    {
      /* CRCs different => Error */
      CRCCorrect = 0;
    }

    if((IrrecoverableErrors == 0) && CRCCorrect)
    {
      fprintf(stderr, "(OK)");
    }
    else if(IrrecoverableErrors == 0)
    {
      fprintf(stderr, "RAS (FEC OK/CRC ERR)");
    }
    else fprintf(stderr, "(FEC FAIL/CRC ERR)");
  }
  else
  {
    if(IrrecoverableErrors == 0) fprintf(stderr, "(OK)");
    else fprintf(stderr, "(FEC FAIL/CRC ERR)");
  }

#ifdef PRINT_RATE_34_DATA_BYTES
  fprintf(stderr, "\n");
  fprintf(stderr, "RATE 3/4 DATA : ");
  for(i = 0; i < 18; i++)
  {
    fprintf(stderr, "0x%02X", DmrDataByte[i]);
    if(i != 17) fprintf(stderr, " - ");
  }

  fprintf(stderr, "\n");

  fprintf(stderr, "CRC extracted  = 0x%08X - CRC computed  = 0x%08X - ", CRCExtracted, CRCComputed);

  if((IrrecoverableErrors == 0) && CRCCorrect)
  {
    fprintf(stderr, "CRCs are equal + FEC OK !\n");
  }
  else if(IrrecoverableErrors == 0)
  {
    fprintf(stderr, "FEC correctly corrected but CRCs are incorrect\n");
  }
  else
  {
    fprintf(stderr, "ERROR !!! CRCs are different and FEC Failed !\n");
  }

  fprintf(stderr, "Hamming Irrecoverable Errors = %u\n", IrrecoverableErrors);
#endif /* PRINT_RATE_34_DATA_BYTES */
} /* End ProcessDmr34Data() */


#if defined(BUILD_DSD_WITH_HYTERA_BASIC_PRIVACY)
/* Set the encryption type based on the FID */
void SetDmrEncryptionType(dsd_opts * opts, dsd_state * state)
{
  TimeSlotVoiceSuperFrame_t * TSVoiceSupFrame = NULL;

  /* Remove warning compiler */
  UNUSED_VARIABLE(opts);

  /* Check the current time slot */
  if(state->currentslot == 0)
  {
    TSVoiceSupFrame = &state->TS1SuperFrame;
  }
  else
  {
    TSVoiceSupFrame = &state->TS2SuperFrame;
  }

  if(TSVoiceSupFrame->FullLC.DataValidity)
  {
    switch(TSVoiceSupFrame->FullLC.FeatureSetID)
    {
      /* FID = 0x68 ==> Hytera Basic Privacy */
      case 0x68:
      {
        /* Check if the Hytera 40 bit basic key is used */
        if(opts->dmr_Hytera_basic_enc_key_size == 40)
        {
          opts->EncryptionMode = MODE_HYTERA_BASIC_40_BIT;
        }
        /* Check if the Hytera 128 bit basic key is used */
        else if(opts->dmr_Hytera_basic_enc_key_size == 128)
        {
          opts->EncryptionMode = MODE_HYTERA_BASIC_128_BIT;
        }
        /* Check if the Hytera 256 bit basic key is used */
        else if(opts->dmr_Hytera_basic_enc_key_size == 256)
        {
          opts->EncryptionMode = MODE_HYTERA_BASIC_256_BIT;
        }
        else
        {
          /* 40 bits key by default */
          opts->EncryptionMode = MODE_HYTERA_BASIC_40_BIT;
        }
        break;
      } /* End case 0x68 */

      default:
      {
        /* Unknown FID, nothing to do */
        break;
      }
    }
  } /* End if(TSVoiceSupFrame->FullLC.DataValidity) */
} /* End SetDmrEncryptionType() */
#endif /* #if defined(BUILD_DSD_WITH_HYTERA_BASIC_PRIVACY) */


/*
 * @brief : This function compute the CRC-CCITT of the DMR data
 *          by using the polynomial x^16 + x^12 + x^5 + 1
 *
 * @param Input : A buffer pointer of the DMR data (80 bytes)
 *
 * @return The 16 bit CRC
 */
uint16_t ComputeCrcCCITT(uint8_t * DMRData)
{
  uint32_t i;
  uint16_t CRC = 0x0000; /* Initialization value = 0x0000 */
  /* Polynomial x^16 + x^12 + x^5 + 1
   * Normal     = 0x1021
   * Reciprocal = 0x0811
   * Reversed   = 0x8408
   * Reversed reciprocal = 0x8810 */
  uint16_t Polynome = 0x1021;
  for(i = 0; i < 80; i++)
  {
    if(((CRC >> 15) & 1) ^ (DMRData[i] & 1))
    {
      CRC = (CRC << 1) ^ Polynome;
    }
    else
    {
      CRC <<= 1;
    }
  }

  /* Invert the CRC */
  CRC ^= 0xFFFF;

  /* Return the CRC */
  return CRC;
} /* End ComputeCrcCCITT() */


/*
 * @brief : This function compute the CRC-24 bit of the full
 *          link control by using the Reed-Solomon(12,9) FEC
 *
 * @param FullLinkControlDataBytes : A buffer pointer of the DMR data bytes (12 bytes)
 *
 * @param CRCComputed : A 32 bit pointer where the computed CRC 24-bit will be stored
 *
 * @param CRCMask : The 24 bit CRC mask to apply
 *
 * @return 0 = CRC error
 *         1 = CRC is correct
 */
uint32_t ComputeAndCorrectFullLinkControlCrc(uint8_t * FullLinkControlDataBytes, uint32_t * CRCComputed, uint32_t CRCMask)
{
  uint32_t i;
  rs_12_9_codeword_t VoiceLCHeaderStr;
  rs_12_9_poly_t syndrome;
  uint8_t errors_found = 0;
  rs_12_9_correct_errors_result_t result = RS_12_9_CORRECT_ERRORS_RESULT_NO_ERRORS_FOUND;
  uint32_t CrcIsCorrect = 0;

  for(i = 0; i < 12; i++)
  {
    VoiceLCHeaderStr.data[i] = FullLinkControlDataBytes[i];

    /* Apply CRC mask on each 3 last bytes
     * of the full link control */
    if(i == 9)
    {
      VoiceLCHeaderStr.data[i] ^= (uint8_t)(CRCMask >> 16);
    }
    else if(i == 10)
    {
      VoiceLCHeaderStr.data[i] ^= (uint8_t)(CRCMask >> 8);
    }
    else if(i == 11)
    {
      VoiceLCHeaderStr.data[i] ^= (uint8_t)(CRCMask);
    }
    else
    {
      /* Nothing to do */
    }
  }

  /* Check and correct the full link LC control with Reed Solomon (12,9) FEC */
  rs_12_9_calc_syndrome(&VoiceLCHeaderStr, &syndrome);
  if(rs_12_9_check_syndrome(&syndrome) != 0) result = rs_12_9_correct_errors(&VoiceLCHeaderStr, &syndrome, &errors_found);

  /* Reconstitue the CRC */
  *CRCComputed  = (uint32_t)((VoiceLCHeaderStr.data[9]  << 16) & 0xFF0000);
  *CRCComputed |= (uint32_t)((VoiceLCHeaderStr.data[10] <<  8) & 0x00FF00);
  *CRCComputed |= (uint32_t)((VoiceLCHeaderStr.data[11] <<  0) & 0x0000FF);

  if((result == RS_12_9_CORRECT_ERRORS_RESULT_NO_ERRORS_FOUND) ||
     (result == RS_12_9_CORRECT_ERRORS_RESULT_ERRORS_CORRECTED))
  {
    //fprintf(stderr, "CRC OK : 0x%06X\n", *CRCComputed);
    CrcIsCorrect = 1;

    /* Reconstitue full link control data after FEC correction */
    for(i = 0; i < 12; i++)
    {
      FullLinkControlDataBytes[i] = VoiceLCHeaderStr.data[i];

      /* Apply CRC mask on each 3 last bytes
       * of the full link control */
      if(i == 9)
      {
        FullLinkControlDataBytes[i] ^= (uint8_t)(CRCMask >> 16);
      }
      else if(i == 10)
      {
        FullLinkControlDataBytes[i] ^= (uint8_t)(CRCMask >> 8);
      }
      else if(i == 11)
      {
        FullLinkControlDataBytes[i] ^= (uint8_t)(CRCMask);
      }
      else
      {
        /* Nothing to do */
      }
    }
  }
  else
  {
    //fprintf(stderr, "CRC ERROR : 0x%06X\n", *CRCComputed);
    CrcIsCorrect = 0;
  }

  /* Return the CRC status */
  return CrcIsCorrect;
} /* End ComputeAndCorrectFullLinkControlCrc() */


/*
 * @brief : This function compute the 5 bit CRC of the DMR voice burst data
 *          See ETSI TS 102 361-1 chapter B.3.11
 *
 * @param Input : A buffer pointer of the DMR data (72 bytes)
 *
 * @return The 5 bit CRC
 */
uint8_t ComputeCrc5Bit(uint8_t * DMRData)
{
  uint32_t i, j, k;
  uint8_t  Buffer[9];
  uint32_t Sum;
  uint8_t  CRC = 0;

  /* Convert the 72 bit into 9 bytes */
  k = 0;
  for(i = 0; i < 9; i++)
  {
    Buffer[i] = 0;
    for(j = 0; j < 8; j++)
    {
      Buffer[i] = Buffer[i] << 1;
      Buffer[i] = Buffer[i] | DMRData[k++];
    }
  }

  /* Add all 9 bytes */
  Sum = 0;
  for(i = 0; i < 9; i++)
  {
    Sum += (uint32_t)Buffer[i];
  }

  /* Sum MOD 31 = CRC */
  CRC = (uint8_t)(Sum % 31);

  /* Return the CRC */
  return CRC;
} /* End ComputeCrc5Bit() */


/*
 * @brief : This function compute the CRC-9 of the DMR data
 *          by using the polynomial x^9 + x^6 + x^4 + x^3 + 1
 *
 * @param Input : A buffer pointer of the DMR data (80 bytes)
 *        Rate 1/2 coded confirmed (10 data octets): 80 bit sequence (80 bytes)
 *        Rate 3/4 coded confirmed (16 data octets): 128 bit sequence (120 bytes)
 *        Rate 1 coded confirmed (22 data octets): 176 bit sequence (176 bytes)
 *
 * @param NbData : The number of bit to compute
 *        Rate 1/2 coded confirmed (10 data octets): 80 bit sequence (80 bytes)
 *        Rate 3/4 coded confirmed (16 data octets): 128 bit sequence (120 bytes)
 *        Rate 1 coded confirmed (22 data octets): 176 bit sequence (176 bytes)
 *
 * @return The 9 bit CRC
 */
uint16_t ComputeCrc9Bit(uint8_t * DMRData, uint32_t NbData)
{
  uint32_t i;
  uint16_t CRC = 0x0000; /* Initialization value = 0x0000 */
  /* Polynomial x^9 + x^6 + x^4 + x^3 + 1
   * Normal     = 0x059
   * Reciprocal = 0x134
   * Reversed reciprocal = 0x12C */
  uint16_t Polynome = 0x059;
  for(i = 0; i < NbData; i++)
  {
    if(((CRC >> 8) & 1) ^ (DMRData[i] & 1))
    {
      CRC = (CRC << 1) ^ Polynome;
    }
    else
    {
      CRC <<= 1;
    }
  }

  /* Invert the CRC */
  CRC ^= 0x01FF;

  /* Return the CRC */
  return CRC;
} /* End ComputeCrc9Bit() */


/*
 * @brief : This function compute the CRC-32 of the DMR data
 *          by using the polynomial x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1
 *
 * @param Input : A buffer pointer of the DMR data (80 bytes)
 *        Rate 1/2 coded confirmed (10 data octets): 80 bit sequence (80 bytes)
 *        Rate 3/4 coded confirmed (16 data octets): 128 bit sequence (120 bytes)
 *        Rate 1 coded confirmed (22 data octets): 176 bit sequence (176 bytes)
 *
 * @param NbData : The number of bit to compute
 *        Rate 1/2 coded confirmed (10 data octets): 80 bit sequence (80 bytes)
 *        Rate 3/4 coded confirmed (16 data octets): 128 bit sequence (120 bytes)
 *        Rate 1 coded confirmed (22 data octets): 176 bit sequence (176 bytes)
 *
 * @return The 32 bit CRC
 */
uint32_t ComputeCrc32Bit(uint8_t * DMRData, uint32_t NbData)
{
  uint32_t i;
  uint32_t CRC = 0x00000000; /* Initialization value = 0x00000000 */
  /* Polynomial x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1
   * Normal     = 0x04C11DB7
   * Reversed   = 0xEDB88320
   * Reciprocal = 0xDB710641
   * Reversed reciprocal = 0x82608EDB */
  uint32_t Polynome = 0x04C11DB7;
  for(i = 0; i < NbData; i++)
  {
    if(((CRC >> 31) & 1) ^ (DMRData[i] & 1))
    {
      CRC = (CRC << 1) ^ Polynome;
    }
    else
    {
      CRC <<= 1;
    }
  }

  /* Return the CRC */
  return CRC;
} /* End ComputeCrc32Bit() */


/*
 * @brief : This function returns the Algorithm ID into an explicit string
 *
 * @param AlgID : The algorithm ID
 *   @arg : 0x21 for ARC4
 *   @arg : 0x25 for AES256
 *
 * @return A constant string pointer that explain the Alg ID used
 */
uint8_t * DmrAlgIdToStr(uint8_t AlgID)
{
  if(AlgID == 0x21) return (uint8_t *)"ARC4";
  else if(AlgID == 0x25) return (uint8_t *)"AES256";
  else return (uint8_t *)"UNKNOWN";
} /* End DmrAlgIdToStr */


/*
 * @brief : This function returns the encryption mode into an explicit string
 *
 * @param PrivacyMode : The algorithm ID
 *   @arg : MODE_UNENCRYPTED
 *   @arg : MODE_BASIC_PRIVACY
 *   @arg : MODE_ENHANCED_PRIVACY_ARC4
 *   @arg : MODE_ENHANCED_PRIVACY_DES
 *   @arg : MODE_ENHANCED_PRIVACY_AES256
 *   @arg : MODE_HYTERA_BASIC_40_BIT
 *   @arg : MODE_HYTERA_BASIC_128_BIT
 *   @arg : MODE_HYTERA_BASIC_256_BIT
 *
 * @return A constant string pointer that explain the encryption mode used
 */
uint8_t * DmrAlgPrivacyModeToStr(uint32_t PrivacyMode)
{
  switch(PrivacyMode)
  {
    case MODE_UNENCRYPTED:
    {
      return (uint8_t *)"NOT ENC";
      break;
    }
    case MODE_BASIC_PRIVACY:
    {
      return (uint8_t *)"BP";
      break;
    }
    case MODE_ENHANCED_PRIVACY_ARC4:
    {
      return (uint8_t *)"EP ARC4";
      break;
    }
    case MODE_ENHANCED_PRIVACY_AES256:
    {
      return (uint8_t *)"EP AES256";
      break;
    }
    case MODE_HYTERA_BASIC_40_BIT:
    {
      return (uint8_t *)"HYTERA BASIC 40 BIT";
      break;
    }
    case MODE_HYTERA_BASIC_128_BIT:
    {
      return (uint8_t *)"HYTERA BASIC 128 BIT";
      break;
    }
    case MODE_HYTERA_BASIC_256_BIT:
    {
      return (uint8_t *)"HYTERA BASIC 256 BIT";
      break;
    }
    default:
    {
      return (uint8_t *)"UNKNOWN";
      break;
    }
  } /* End switch(PrivacyMode) */
} /* End DmrAlgPrivacyModeToStr() */


/* End of file */
