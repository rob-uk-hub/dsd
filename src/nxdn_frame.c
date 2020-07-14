/*
 ============================================================================
 Name        : nxdn_frame
 Author      : Louis-Erig HERVE - F4HUZ
 Version     : 1.0
 Date        : 2020 July 10
 Copyright   : No copyright
 Description : NXDN frame decoding source lib
 ============================================================================
 */


/* Include ------------------------------------------------------------------*/
#include "dsd.h"
#include "nxdn_const.h"


/* Global variables ---------------------------------------------------------*/


/* Functions ----------------------------------------------------------------*/


/*
 * @brief : This function decodes an NXDN frame
 *
 * @param opts : Options
 *
 * @param state : State
 *
 * @param Inverted : 1 = Inverted frame ; 0 = Normal frame
 *
 * @return None
 *
 */
void ProcessNXDNFrame(dsd_opts * opts, dsd_state * state, uint8_t Inverted)
{
  /* Switch to the correct type of frame */
  switch(state->NxdnLich.RFChannelType)
  {
    /* RCCH */
    case 0:
    {
      ProcessNxdnRCCHFrame(opts, state, Inverted);
      break;
    } /* End case 0 */

    /* RTCH */
    case 1:
    {
      ProcessNxdnRTCHFrame(opts, state, Inverted);
      break;
    } /* End case 1 */

    /* RDCH */
    case 2:
    {
      ProcessNxdnRDCHFrame(opts, state, Inverted);
      break;
    } /* End case 2 */

    /* RTCH_C (Composite Control Channel) */
    case 3:
    {
      ProcessNxdnRTCH_C_Frame(opts, state, Inverted);
      break;
    } /* End case 3 */

    default:
    {
      printf("- Unknown NXDN Frame type\n");

      /* Unknown frame type, simply skip the entire frame
       * 384 bits - 20 bits Frame Sync Word - 16 bits LICH = 348 bits = 174 dibit */
      skipDibit(opts, state, (384 - 20 - 16) / 2);
      break;
    } /* End default */
  } /* End switch(state->NxdnLich.RFChannelType) */
} /* End NxdnDecodeFrame() */




/*
 * @brief : This function decodes an NXDN RCCH frame
 *
 * @param opts : Options
 *
 * @param state : State
 *
 * @param Inverted : 1 = Inverted frame ; 0 = Normal frame
 *
 * @return None
 *
 */
void ProcessNxdnRCCHFrame(dsd_opts * opts, dsd_state * state, uint8_t Inverted)
{
  uint8_t SkipEntireFrameFlag = 0;

  /* Remove compiler warning */
  UNUSED_VARIABLE(Inverted);

  sprintf (state->fsubtype, " RCCH         ");

  switch(state->NxdnLich.CompleteLich)
  {
    case 0b0011000:
    {
      printf("- RCCH - CCCH Transmission using Short CAC\n");
      SkipEntireFrameFlag = 1;
      break;
    }
    case 0b0001000:
    {
      printf("- RCCH - UPCH or CCCH Transmission using Long CAC\n");
      SkipEntireFrameFlag = 1;
      break;
    }
    case 0b0000001:
    {
      printf("- RCCH - CAC Transmission\n");
      SkipEntireFrameFlag = 1;
      break;
    }
    case 0b0000011:
    {
      printf("- RCCH - CAC Transmission which has not to be received\n");
      SkipEntireFrameFlag = 1;
      break;
    }
    case 0b0000101:
    {
      printf("- RCCH - CAC Transmission which can be received arbitrary\n");
      SkipEntireFrameFlag = 1;
      break;
    }
    default:
    {
      printf("- RCCH - Unknown RCCH Frame\n");
      SkipEntireFrameFlag = 1;
      break;
    } /* End default */
  } /* switch(state->NxdnLich.CompleteLich) */

  if(SkipEntireFrameFlag)
  {
    /* Unknown frame type or frame not implemented yet, simply skip the entire frame
     * 384 bits - 20 bits Frame Sync Word - 16 bits LICH = 348 bits = 174 dibit */
    skipDibit(opts, state, (384 - 20 - 16) / 2);
  }

} /* End ProcessNxdnRCCHFrame() */




/*
 * @brief : This function decodes an NXDN RTCH frame
 *
 * @param opts : Options
 *
 * @param state : State
 *
 * @param Inverted : 1 = Inverted frame ; 0 = Normal frame
 *
 * @return None
 *
 */
void ProcessNxdnRTCHFrame(dsd_opts * opts, dsd_state * state, uint8_t Inverted)
{
  uint8_t SkipEntireFrameFlag = 0;

  /* Remove compiler warning */
  UNUSED_VARIABLE(Inverted);

  switch(state->NxdnLich.CompleteLich)
  {
    /* Voice frame */
    case 0b0110110: /* Voice call transmission : Superframe SACCH, VCH Only */
    case 0b0110100: /* Voice call transmission : Superframe SACCH, first half VCH & last half FACCH1 */
    case 0b0110010: /* Voice call transmission : Superframe SACCH, first half FACCH1 & last half VCH */
    case 0b0110111: /* Voice call transmission : Superframe SACCH, VCH Only */
    case 0b0110101: /* Voice call transmission : Superframe SACCH, first half VCH & last half FACCH1 */
    case 0b0110011: /* Voice call transmission : Superframe SACCH, first half FACCH1 & last half VCH */
    {
      printf("- RTCH - ");
      processNXDNVoice(opts, state);
      break;
    }

    case 0b0110000: /* Voice call transmission : Superframe SACCH, two FACCH1s */
    case 0b0110001: /* Voice call transmission : Superframe SACCH, two FACCH1s */
    {
      printf("- RTCH - ");
      processNXDNData(opts, state);
      break;
    }

    case 0b0111000: /* Voice call transmission : Superframe SACCH, Idle State  */
    case 0b0111001: /* Voice call transmission : Superframe SACCH, Idle State  */
    {
      printf("- RTCH - ");
      ProcessNXDNIdleData(opts, state, Inverted);
      break;
    }

    case 0b0100000: /* Voice call transmission : Non-superframe SACCH, two FACCH1s */
    case 0b0100001: /* Voice call transmission : Non-superframe SACCH, two FACCH1s */
    {
      //printf("- RTCH - Non-superframe SACCH with two FACCH1s\n");
      printf("- RDCH - ");
      ProcessNXDNFacch1Data(opts, state, Inverted);
      break;
    }

    case 0b0101110: /* UDCH Transmission */
    case 0b0101111: /* UDCH Transmission */
    {
      printf("- RTCH - UDCH Transmission\n");
      SkipEntireFrameFlag = 1;
      break;
    }

    case 0b0101000: /* FACCH2 Transmission */
    case 0b0101001: /* FACCH2 Transmission */
    {
      printf("- RTCH - FACCH2 Transmission\n");
      SkipEntireFrameFlag = 1;
      break;
    }
    default:
    {
      printf("- RTCH - Unknown RTCH Frame\n");
      SkipEntireFrameFlag = 1;
      break;
    } /* End default */
  } /* End switch(state->NxdnLich.CompleteLich) */

  if(SkipEntireFrameFlag)
  {
    /* Unknown frame type or frame not implemented yet, simply skip the entire frame
     * 384 bits - 20 bits Frame Sync Word - 16 bits LICH = 348 bits = 174 dibit */
    skipDibit(opts, state, (384 - 20 - 16) / 2);
  }
} /* End ProcessNxdnRTCHFrame() */




/*
 * @brief : This function decodes an NXDN RDCH frame
 *
 * @param opts : Options
 *
 * @param state : State
 *
 * @param Inverted : 1 = Inverted frame ; 0 = Normal frame
 *
 * @return None
 *
 */
void ProcessNxdnRDCHFrame(dsd_opts * opts, dsd_state * state, uint8_t Inverted)
{
  uint8_t SkipEntireFrameFlag = 0;

  /* Remove compiler warning */
  UNUSED_VARIABLE(Inverted);

  switch(state->NxdnLich.CompleteLich)
  {
    /* Voice frame */
    case 0b1010110: /* Voice call transmission : Superframe SACCH, VCH Only */
    case 0b1010100: /* Voice call transmission : Superframe SACCH, first half VCH & last half FACCH1 */
    case 0b1010010: /* Voice call transmission : Superframe SACCH, first half FACCH1 & last half VCH */
    case 0b1010111: /* Voice call transmission : Superframe SACCH, VCH Only */
    case 0b1010101: /* Voice call transmission : Superframe SACCH, first half VCH & last half FACCH1 */
    case 0b1010011: /* Voice call transmission : Superframe SACCH, first half FACCH1 & last half VCH */
    {
      printf("- RDCH - ");
      processNXDNVoice(opts, state);
      break;
    }

    case 0b1010000: /* Voice call transmission : Superframe SACCH, two FACCH1s */
    case 0b1010001: /* Voice call transmission : Superframe SACCH, two FACCH1s */
    {
      printf("- RDCH - ");
      processNXDNData(opts, state);
      break;
    }

    case 0b1011000: /* Voice call transmission : Superframe SACCH, Idle State  */
    case 0b1011001: /* Voice call transmission : Superframe SACCH, Idle State  */
    {
      printf("- RDCH - ");
      ProcessNXDNIdleData(opts, state, Inverted);
      break;
    }

    case 0b1000000: /* Voice call transmission : Non-superframe SACCH, two FACCH1s */
    case 0b1000001: /* Voice call transmission : Non-superframe SACCH, two FACCH1s */
    {
      //printf("- RDCH - Non-superframe SACCH with two FACCH1s\n");
      printf("- RDCH - ");
      ProcessNXDNFacch1Data(opts, state, Inverted);
      break;
    }

    case 0b1001110: /* UDCH Transmission */
    case 0b1001111: /* UDCH Transmission */
    {
      printf("- RDCH - UDCH Transmission\n");
      SkipEntireFrameFlag = 1;
      break;
    }

    case 0b1001000: /* FACCH2 Transmission */
    case 0b1001001: /* FACCH2 Transmission */
    {
      printf("- RDCH - FACCH2 Transmission\n");
      SkipEntireFrameFlag = 1;
      break;
    }

    default:
    {
      printf("- RDCH - Unknown RDCH Frame\n");
      SkipEntireFrameFlag = 1;
      break;
    } /* End default */
  } /* End switch(state->NxdnLich.CompleteLich) */

  if(SkipEntireFrameFlag)
  {
    /* Unknown frame type or frame not implemented yet, simply skip the entire frame
     * 384 bits - 20 bits Frame Sync Word - 16 bits LICH = 348 bits = 174 dibit */
    skipDibit(opts, state, (384 - 20 - 16) / 2);
  }
} /* End ProcessNxdnRDCHFrame() */




/*
 * @brief : This function decodes an NXDN RTCH_C frame (Composite control channel)
 *
 * @param opts : Options
 *
 * @param state : State
 *
 * @param Inverted : 1 = Inverted frame ; 0 = Normal frame
 *
 * @return None
 *
 */
void ProcessNxdnRTCH_C_Frame(dsd_opts * opts, dsd_state * state, uint8_t Inverted)
{
  uint8_t SkipEntireFrameFlag = 0;

  /* Remove compiler warning */
  UNUSED_VARIABLE(Inverted);

  switch(state->NxdnLich.CompleteLich)
  {
    /* Voice frame */
    case 0b1110111: /* Voice call transmission : Superframe SACCH, VCH Only */
    case 0b1110101: /* Voice call transmission : Superframe SACCH, first half VCH & last half FACCH1 */
    case 0b1110011: /* Voice call transmission : Superframe SACCH, first half FACCH1 & last half VCH */
    {
      printf("- RTCH_C - ");
      processNXDNVoice(opts, state);
      break;
    }

    case 0b1110001: /* Voice call transmission : Superframe SACCH, two FACCH1s */
    {
      printf("- RTCH_C - ");
      processNXDNData(opts, state);
      break;
    }

    case 0b1111001: /* Voice call transmission : Superframe SACCH, Idle State  */
    {
      printf("- RTCH_C - ");
      ProcessNXDNIdleData(opts, state, Inverted);
      break;
    }

    case 0b1100001: /* Voice call transmission : Non-superframe SACCH, two FACCH1s */
    {
      //printf("- RTCH_C - Non-superframe SACCH with two FACCH1s\n");
      printf("- RTCH_C - ");
      ProcessNXDNFacch1Data(opts, state, Inverted);
      break;
    }

    case 0b1101111: /* UDCH Transmission */
    {
      printf("- RTCH_C - UDCH Transmission\n");
      SkipEntireFrameFlag = 1;
      break;
    }

    case 0b1101001: /* FACCH2 Transmission */
    {
      printf("- RTCH_C - FACCH2 Transmission\n");
      SkipEntireFrameFlag = 1;
      break;
    }

    default:
    {
      printf("- RTCH_C - Unknown RTCH_C Frame\n");
      SkipEntireFrameFlag = 1;
      break;
    } /* End default */
  } /* End switch(state->NxdnLich.CompleteLich) */

  if(SkipEntireFrameFlag)
  {
    /* Unknown frame type or frame not implemented yet, simply skip the entire frame
     * 384 bits - 20 bits Frame Sync Word - 16 bits LICH = 348 bits = 174 dibit */
    skipDibit(opts, state, (384 - 20 - 16) / 2);
  }
} /* End ProcessNxdnRDCHFrame() */




/*
 * @brief : This function decodes an NXDN RCCH Idle Data frame
 *
 * @param opts : Options
 *
 * @param state : State
 *
 * @param Inverted : 1 = Inverted frame ; 0 = Normal frame
 *
 * @return None
 *
 */
void ProcessNXDNIdleData (dsd_opts * opts, dsd_state * state, uint8_t Inverted)
{
  int i, dibit = 0;
  unsigned char sacch_raw[72] = {0};
  unsigned char sacch_decoded[32] = {0}; /* 26 bit + 6 bit CRC */
  unsigned char *pr;
  uint8_t CrcIsGood = 0;
  uint8_t StructureField = 0;
  uint8_t RAN = 0;
  uint8_t PartOfFrame = 0;

  /* Remove compiler warning */
  UNUSED_VARIABLE(dibit);
  UNUSED_VARIABLE(Inverted);

  if (opts->errorbars == 1)
  {
    printf ("DATA  - ");
  }

  /* Start pseudo-random NXDN sequence after
   * LITCH = 16 bit = 8 dibit
   * ==> Index 8 */
  pr = (unsigned char *)(&nxdnpr[8]);
  for (i = 0; i < 30; i++)
  {
    dibit = getDibit (opts, state);

    /* Store and de-interleave directly SACCH bits */
    sacch_raw[(2*i)]   = ((*pr) & 1) ^ (1 & (dibit >> 1));  // bit 1
    sacch_raw[(2*i)+1] = (1 & dibit);               // bit 0
    pr++;
#ifdef NXDN_DUMP
    printf ("%c", dibit + 48);
#endif
  }
#ifdef NXDN_DUMP
  printf (" ");
#endif


  /* Decode the SACCH fields */
  CrcIsGood = NXDN_SACCH_raw_part_decode(sacch_raw, sacch_decoded);
  StructureField = (sacch_decoded[0]<<1 | sacch_decoded[1]<<0) & 0x03; /* Compute the Structure Field (remove 2 first bits of the SR Information in the SACCH) */
  RAN = (sacch_decoded[2]<<5 | sacch_decoded[3]<<4 | sacch_decoded[4]<<3 | sacch_decoded[5]<<2 | sacch_decoded[6]<<1 | sacch_decoded[7]<<0) & 0x3F; /* Compute the RAN (6 last bits of the SR Information in the SACCH) */

  /* Compute the Part of Frame according to the structure field */
  if     (StructureField == 3) PartOfFrame = 0;
  else if(StructureField == 2) PartOfFrame = 1;
  else if(StructureField == 1) PartOfFrame = 2;
  else if(StructureField == 0) PartOfFrame = 3;
  else PartOfFrame = 0; /* Normally we should never go here */

  /* Fill the SACCH structure */
  state->NxdnSacchRawPart[PartOfFrame].CrcIsGood = CrcIsGood;
  state->NxdnSacchRawPart[PartOfFrame].StructureField = StructureField;
  state->NxdnSacchRawPart[PartOfFrame].RAN = RAN;
  memcpy(state->NxdnSacchRawPart[PartOfFrame].Data, &sacch_decoded[8], 18); /* Copy the 18 bits of SACCH content */

  printf("RAN=%02d - Part %d/4 ", RAN, PartOfFrame + 1);
  if(CrcIsGood) printf("   (OK)   - ");
  else printf("(CRC ERR) - ");


  /* This frame is an idle frame */
  printf("Idle");


  /* Decode the SACCH only when all 4 voice frame
   * SACCH spare parts have been received */
  if(PartOfFrame == 3)
  {
    /* Decode the entire SACCH content */
    NXDN_SACCH_Full_decode(opts, state);
  }

  for (i = 0; i < 144; i++)
  {
    dibit = getDibit (opts, state);
#ifdef NXDN_DUMP
    printf ("%c", dibit + 48);
#endif
  }

  if (opts->errorbars == 1)
  {
    printf ("\n");
  }

  /* Reset the SACCH CRCs when all 4 voice frame
   * SACCH spare parts have been received */
  if(PartOfFrame == 3)
  {
    /* Reset all CRCs of the SACCH */
    for(i = 0; i < 4; i++) state->NxdnSacchRawPart[i].CrcIsGood = 0;
  }
} /* End ProcessNXDNIdleData() */




/*
 * @brief : This function decodes an NXDN data frame with FACCH1 fields
 *
 * @param opts : Options
 *
 * @param state : State
 *
 * @param Inverted : 1 = Inverted frame ; 0 = Normal frame
 *
 * @return None
 *
 */
void ProcessNXDNFacch1Data (dsd_opts * opts, dsd_state * state, uint8_t Inverted)
{
  int i, j, dibit = 0;
  unsigned char sacch_raw[72] = {0};
  unsigned char sacch_decoded[32] = {0}; /* 26 bits + 6 bits CRC */
  unsigned char facch1_raw[144] = {0};
  unsigned char facch1_decoded[96] = {0}; /* 80 bits + 12 bits CRC + 4 bits "Tail" (set to "0") */
  unsigned char *pr;
  uint8_t CrcIsGood = 0;
  uint8_t StructureField = 0;
  uint8_t RAN = 0;
  uint8_t PartOfFrame = 0;
  uint8_t Facch1CrcIsGood = 0;

  /* Remove compiler warning */
  UNUSED_VARIABLE(dibit);
  UNUSED_VARIABLE(Inverted);

  if (opts->errorbars == 1)
  {
    printf ("DATA  - ");
  }

  /* Start pseudo-random NXDN sequence after
   * LITCH = 16 bit = 8 dibit
   * ==> Index 8 */
  pr = (unsigned char *)(&nxdnpr[8]);
  for (i = 0; i < 30; i++)
  {
    dibit = getDibit (opts, state);

    /* Store and de-interleave directly SACCH bits */
    sacch_raw[(2*i)]   = ((*pr) & 1) ^ (1 & (dibit >> 1));  // bit 1
    sacch_raw[(2*i)+1] = (1 & dibit);               // bit 0
    pr++;
#ifdef NXDN_DUMP
    printf ("%c", dibit + 48);
#endif
  }
#ifdef NXDN_DUMP
  printf (" ");
#endif


  /* Decode the SACCH fields */
  CrcIsGood = NXDN_SACCH_raw_part_decode(sacch_raw, sacch_decoded);
  StructureField = (sacch_decoded[0]<<1 | sacch_decoded[1]<<0) & 0x03; /* Compute the Structure Field (remove 2 first bits of the SR Information in the SACCH) */
  RAN = (sacch_decoded[2]<<5 | sacch_decoded[3]<<4 | sacch_decoded[4]<<3 | sacch_decoded[5]<<2 | sacch_decoded[6]<<1 | sacch_decoded[7]<<0) & 0x3F; /* Compute the RAN (6 last bits of the SR Information in the SACCH) */

  /* Compute the Part of Frame according to the structure field */
  if     (StructureField == 3) PartOfFrame = 0;
  else if(StructureField == 2) PartOfFrame = 1;
  else if(StructureField == 1) PartOfFrame = 2;
  else if(StructureField == 0) PartOfFrame = 3;
  else PartOfFrame = 0; /* Normally we should never go here */

  /* Fill the SACCH structure */
  state->NxdnSacchRawPart[PartOfFrame].CrcIsGood = CrcIsGood;
  state->NxdnSacchRawPart[PartOfFrame].StructureField = StructureField;
  state->NxdnSacchRawPart[PartOfFrame].RAN = RAN;
  memcpy(state->NxdnSacchRawPart[PartOfFrame].Data, &sacch_decoded[8], 18); /* Copy the 18 bits of SACCH content */

  printf("RAN=%02d - Part %d/4 ", RAN, PartOfFrame + 1);
  if(CrcIsGood) printf("   (OK)   - ");
  else printf("(CRC ERR) - ");


  /* This frame is an FACCH1 frame */
  printf("FACCH1 - ");


  /* Decode the SACCH only when all 4 voice frame
   * SACCH spare parts have been received */
  if(PartOfFrame == 3)
  {
    /* Decode the entire SACCH content */
    //NXDN_SACCH_Full_decode(opts, state);
  }

  /* Start pseudo-random NXDN sequence after
   * LITCH = 16 bit = 8 dibit +
   * SACCH = 60 bit = 30 dibit
   * = 76 bit = 38 dibit
   * ==> Index 38 */
  pr = (unsigned char *)(&nxdnpr[38]);

  /* The frame contains 2 * 144 bits = 2 * 72 dibit */
  for(j = 0; j < 2; j++)
  {
    for (i = 0; i < 72; i++)
    {
      dibit = getDibit (opts, state);

      /* Store and FACCH1 bits */
      facch1_raw[(2*i)]   = ((*pr) & 1) ^ (1 & (dibit >> 1));  // bit 1
      facch1_raw[(2*i)+1] = (1 & dibit);               // bit 0
      pr++;

#ifdef NXDN_DUMP
    printf ("%c", dibit + 48);
#endif
    } /* End for (i = 0; i < 72; i++) */

    /* Extract the FACCH1 content */
    Facch1CrcIsGood = NXDN_FACCH1_decode(facch1_raw, facch1_decoded);

    state->NxdnFacch1RawPart[j].CrcIsGood = Facch1CrcIsGood;
    memcpy(state->NxdnFacch1RawPart[j].Data, facch1_decoded, 80);

    if(Facch1CrcIsGood) printf("FACCH1 CRC %d is good - ", j + 1);
    else printf("FACCH1 CRC %d is bad - ", j + 1);
  } /* End for(j = 0; j < 2; j++) */

  if (opts->errorbars == 1)
  {
    printf ("\n");
  }

  /* Reset the SACCH CRCs when all 4 voice frame
   * SACCH spare parts have been received */
  if(PartOfFrame == 3)
  {
    /* Reset all CRCs of the SACCH */
    for(i = 0; i < 4; i++) state->NxdnSacchRawPart[i].CrcIsGood = 0;
  }
} /* End ProcessNXDNFacch1Data() */




/* End of file */
