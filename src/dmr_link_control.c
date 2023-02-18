/*
 ============================================================================
 Name        : dmr_link_control.c
 Author      : Louis-Erig HERVE - F4HUZ
 Version     : V1.0
 Date        : 2022 October 16th
 Copyright   : None
 Description : DMR Link Control management library
 ============================================================================
 */

/* Include ------------------------------------------------------------------*/
#include "dsd.h"


/* Global variables ---------------------------------------------------------*/
unsigned int  g_TS1_TalkerAliasHeaderDataFormat = 0;
unsigned int  g_TS1_TalkerAliasHeaderDataLength = 0;
unsigned char g_TS1_TalkerAliasHeaderData[49] = {0};
unsigned char g_TS1_TalkerAliasBlock1Data[56] = {0};
unsigned char g_TS1_TalkerAliasBlock2Data[56] = {0};
unsigned char g_TS1_TalkerAliasBlock3Data[56] = {0};

unsigned int  g_TS2_TalkerAliasHeaderDataFormat = 0;
unsigned int  g_TS2_TalkerAliasHeaderDataLength = 0;
unsigned char g_TS2_TalkerAliasHeaderData[49] = {0};
unsigned char g_TS2_TalkerAliasBlock1Data[56] = {0};
unsigned char g_TS2_TalkerAliasBlock2Data[56] = {0};
unsigned char g_TS2_TalkerAliasBlock3Data[56] = {0};


/* Functions ----------------------------------------------------------------*/

/*
 * @brief : This function initialize the library witch manages the Link Control data
 *
 * @param None
 *
 * @return None
 */
void DmrLinkControlInitLib(void)
{
  /* Init talker alias data (Time Slot 1) */
  g_TS1_TalkerAliasHeaderDataFormat = 0;
  g_TS1_TalkerAliasHeaderDataLength = 0;
  memset(g_TS1_TalkerAliasHeaderData, 0, sizeof(g_TS1_TalkerAliasHeaderData));
  memset(g_TS1_TalkerAliasBlock1Data, 0, sizeof(g_TS1_TalkerAliasBlock1Data));
  memset(g_TS1_TalkerAliasBlock2Data, 0, sizeof(g_TS1_TalkerAliasBlock2Data));
  memset(g_TS1_TalkerAliasBlock3Data, 0, sizeof(g_TS1_TalkerAliasBlock3Data));

  /* Init talker alias data (Time Slot 2) */
  g_TS2_TalkerAliasHeaderDataFormat = 0;
  g_TS2_TalkerAliasHeaderDataLength = 0;
  memset(g_TS2_TalkerAliasHeaderData, 0, sizeof(g_TS1_TalkerAliasHeaderData));
  memset(g_TS2_TalkerAliasBlock1Data, 0, sizeof(g_TS1_TalkerAliasBlock1Data));
  memset(g_TS2_TalkerAliasBlock2Data, 0, sizeof(g_TS1_TalkerAliasBlock2Data));
  memset(g_TS2_TalkerAliasBlock3Data, 0, sizeof(g_TS1_TalkerAliasBlock3Data));
}


/*
 * @brief : This function decodes the full link control data (56 bits)
 *          See ETSI TS 102 361-1 §9.1.6 Full Link Control (FULL LC) PDU.
 *          See ETSI TS 102 361-2 §B.2 CSBK Opcode List.
 *          See ETSI TS 102 361-4 §B.1 CSBK/MBC/UDT Opcode List.
 *
 * @param InputLCDataBit : A buffer pointer of the full LC data (96 bytes equivalent of 96 bits)
 *                         72 LC bits + 5 CRC bits (embedded signalling)
 *                         or 72 LC bits + 24 CRC bits (LC header + terminator)
 *                         or 80 LC bits + 16 CRC bits (CSBK)
 *
 * @param FullLCOutputStruct : Output structure pointer where decoded LC data will be written
 *
 * @return None
 */
void DmrFullLinkControlDecode(dsd_opts * opts, dsd_state * state, uint8_t InputLCDataBit[96], int VoiceCallInProgress, int CSBKContent)
{
  uint32_t i;
  unsigned char * DataPointer = NULL;
  char LCDataBytes[12];
  unsigned int Offset = 0;
  unsigned int BitToCheck = 0;
  unsigned int TalkerAliasBlockNumber = 0;
  FullLinkControlPDU_t * FullLCOutputStruct;

  if((InputLCDataBit == NULL) || (opts == NULL) || (state == NULL)) return;

  /* Check the current time slot */
  if(state->currentslot == 0)
  {
    FullLCOutputStruct = &state->TS1SuperFrame.FullLC;
  }
  else
  {
    FullLCOutputStruct = &state->TS2SuperFrame.FullLC;
  }

  /* Store the 96 bits [72 LC bits + 5 bits CRC = 77 bits] or [80 LC bits + 16 bits CRC = 96 bits] of the Link control data */
  for(i = 0; i < 96; i++)
  {
    FullLCOutputStruct->FullLinkData[i] = (char)InputLCDataBit[i];
  }

  /* Store the Full Link Control Opcode (FLCO)  */
  FullLCOutputStruct->FullLinkControlOpcode = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[2], 6);

  /* Decode Full Link Control Opcode (FLCO)
   * Note : It contains the same value than the Full Link CSBK Opcode (CSBKO) */
  switch(FullLCOutputStruct->FullLinkControlOpcode & 0b111111)
  {

    case 0b000000:  /* Grp_V_Ch_Usr PDU content - See ETSI TS 102 361-2 §7.1.1.1 Group Voice Channel User LC PDU */
    case 0b000011:  /* PUU_V_Ch_Usr content - See ETSI TS 102 361-2 §7.1.1.2 Unit to Unit Voice Channel User LC PDU */
    case 0b010000:  /* Determined experimentally */
    case 0b100000:  /* Determined experimentally (on a Cap+ network) */
    case 0b110000:  /* Terminator Data Link Control (TD_LC) - See ETSI TS 102 361-3 §7.1.1.1 Terminator Data Link Control PDU */
    {
      /* Store the Protect Flag (PF) bit */
      FullLCOutputStruct->ProtectFlag = (unsigned int)(InputLCDataBit[0] & 1);

      /* Store the Reserved bit */
      FullLCOutputStruct->Reserved = (unsigned int)(InputLCDataBit[1] & 1);

      /* Store the Full Link Control Opcode (FLCO)  */
      FullLCOutputStruct->FullLinkControlOpcode = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[2], 6);

      /* Store the Feature set ID (FID)  */
      FullLCOutputStruct->FeatureSetID = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[8], 8);

      /* Store the Service Options  */
      FullLCOutputStruct->ServiceOptions = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[16], 8);

      /* Store the Group address (Talk Group) */
      FullLCOutputStruct->GroupAddress = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[24], 24);

      /* Store the Source address */
      FullLCOutputStruct->SourceAddress = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[48], 24);

      /* Print the destination ID (TG) and the source ID */
      fprintf(stderr, "| TG=%u  Src=%u  ", FullLCOutputStruct->GroupAddress, FullLCOutputStruct->SourceAddress);
      break;
    }

    /* GPS Info PDU content - See ETSI TS 102 361-2 §7.1.1.3 GPS Info LC PDU */
    case 0b001000:
    {
      /* Store the Protect Flag (PF) bit */
      FullLCOutputStruct->ProtectFlag = (unsigned int)(InputLCDataBit[0] & 1);

      /* Store the Reserved bit */
      FullLCOutputStruct->Reserved = (unsigned int)(InputLCDataBit[1] & 1);

      /* Store the Full Link Control Opcode (FLCO)  */
      FullLCOutputStruct->FullLinkControlOpcode = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[2], 6);

      /* Store the Feature set ID (FID)  */
      FullLCOutputStruct->FeatureSetID = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[8], 8);

      /* Store a GPS reserved field */
      FullLCOutputStruct->GPSReserved = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[16], 4);

      /* Store the GPS position error */
      FullLCOutputStruct->GPSPositionError = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[20], 3);

      /* Store the GPS latitude */
      FullLCOutputStruct->GPSLatitude = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[23], 25);

      /* Store the GPS longitude */
      FullLCOutputStruct->GPSLongitude = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[48], 24);

      /* Print the GPS position */
      PrintDmrGpsPositionFromLinkControlData(FullLCOutputStruct->GPSLongitude,
                                             FullLCOutputStruct->GPSLatitude,
                                             FullLCOutputStruct->GPSPositionError);
      break;
    }

    /* Talker Alias header Info PDU content - See ETSI TS 102 361-2 §7.1.1.4 Talker Alias header LC PDU */
    case 0b000100:
    {
      if(VoiceCallInProgress)
      {
        /* Store the Protect Flag (PF) bit */
        FullLCOutputStruct->ProtectFlag = (unsigned int)(InputLCDataBit[0] & 1);

        /* Store the Reserved bit */
        FullLCOutputStruct->Reserved = (unsigned int)(InputLCDataBit[1] & 1);

        /* Store the Full Link Control Opcode (FLCO)  */
        FullLCOutputStruct->FullLinkControlOpcode = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[2], 6);

        /* Store the Feature set ID (FID)  */
        FullLCOutputStruct->FeatureSetID = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[8], 8);

        /* Store talker alias header values */
        FullLCOutputStruct->TalkerAliasHeaderDataFormat = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[16], 2);
        FullLCOutputStruct->TalkerAliasHeaderDataLength = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[18], 5);

        /* Initialize the content of others talker alias buffers */
        memset(FullLCOutputStruct->TalkerAliasBlock1Data, 0, sizeof(FullLCOutputStruct->TalkerAliasBlock1Data));
        memset(FullLCOutputStruct->TalkerAliasBlock2Data, 0, sizeof(FullLCOutputStruct->TalkerAliasBlock3Data));
        memset(FullLCOutputStruct->TalkerAliasBlock3Data, 0, sizeof(FullLCOutputStruct->TalkerAliasBlock3Data));

        for(i = 0; i < 49; i++)
        {
          FullLCOutputStruct->TalkerAliasHeaderData[i] = InputLCDataBit[i + 23];
        }

        fprintf(stderr, "| Talker Alias Header  ");
        TalkerAliasBlockNumber = 0;

        /* Decode the talker alias */
        PrintDmrTalkerAliasFromLinkControlData(FullLCOutputStruct->TalkerAliasHeaderDataFormat,
                                               FullLCOutputStruct->TalkerAliasHeaderDataLength,
                                               TalkerAliasBlockNumber,
                                               FullLCOutputStruct->TalkerAliasHeaderData,
                                               FullLCOutputStruct->TalkerAliasBlock1Data,
                                               FullLCOutputStruct->TalkerAliasBlock2Data,
                                               FullLCOutputStruct->TalkerAliasBlock3Data);
      }
      else
      {
        /* Store the Last Block (LB) bit */
        FullLCOutputStruct->LastBlock = (unsigned int)(InputLCDataBit[0] & 1);

        /* Store the Protect Flag (PF) bit */
        FullLCOutputStruct->ProtectFlag = (unsigned int)(InputLCDataBit[1] & 1);

        /* Store the Full Link CSBK Opcode (CSBKO)  */
        FullLCOutputStruct->FullLinkCSBKOpcode = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[2], 6);

        /* Store the Feature set ID (FID)  */
        FullLCOutputStruct->FeatureSetID = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[8], 8);

        /* Store the Service Options  */
        FullLCOutputStruct->ServiceOptions = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[16], 8);

        /* Store the Service Options  */
        FullLCOutputStruct->Reserved = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[24], 8);

        /* Store the Group address (Talk Group) */
        FullLCOutputStruct->GroupAddress = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[32], 24);

        /* Store the Source address */
        FullLCOutputStruct->SourceAddress = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[56], 24);

        /* Print the destination ID (TG) and the source ID */
        fprintf(stderr, "| TG=%u  Src=%u  ", FullLCOutputStruct->GroupAddress, FullLCOutputStruct->SourceAddress);
      }
      break;
    }

    /* Table 7.5: Talker Alias block Info PDU content - See ETSI TS 102 361-2 §7.1.1.5 Talker Alias block LC PDU  */
    case 0b000101:
    case 0b000110:
    case 0b000111:
    {
      if(VoiceCallInProgress)
      {
        /* Store the Protect Flag (PF) bit */
        FullLCOutputStruct->ProtectFlag = (unsigned int)(InputLCDataBit[0] & 1);

        /* Store the Reserved bit */
        FullLCOutputStruct->Reserved = (unsigned int)(InputLCDataBit[1] & 1);

        /* Store the Full Link Control Opcode (FLCO)  */
        FullLCOutputStruct->FullLinkControlOpcode = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[2], 6);

        /* Store the Feature set ID (FID)  */
        FullLCOutputStruct->FeatureSetID = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[8], 8);

        /* Store the Talker Alias block data */
        if((FullLCOutputStruct->FullLinkControlOpcode & 0b111111) == 0b000101)
        {
          DataPointer = FullLCOutputStruct->TalkerAliasBlock1Data;
          fprintf(stderr, "| Talker Alias Block 1 ");
          TalkerAliasBlockNumber = 1;
        }
        else if((FullLCOutputStruct->FullLinkControlOpcode & 0b111111) == 0b000110)
        {
          DataPointer = FullLCOutputStruct->TalkerAliasBlock2Data;
          fprintf(stderr, "| Talker Alias Block 2 ");
          TalkerAliasBlockNumber = 2;
        }
        else
        {
          DataPointer = FullLCOutputStruct->TalkerAliasBlock3Data;
          fprintf(stderr, "| Talker Alias Block 3 ");
          TalkerAliasBlockNumber = 3;
        }

        for(i = 0; i < 56; i++)
        {
          DataPointer[i] = (unsigned char)InputLCDataBit[i + 16];
        }

        /* Decode the talker alias */
        PrintDmrTalkerAliasFromLinkControlData(FullLCOutputStruct->TalkerAliasHeaderDataFormat,
                                               FullLCOutputStruct->TalkerAliasHeaderDataLength,
                                               TalkerAliasBlockNumber,
                                               FullLCOutputStruct->TalkerAliasHeaderData,
                                               FullLCOutputStruct->TalkerAliasBlock1Data,
                                               FullLCOutputStruct->TalkerAliasBlock2Data,
                                               FullLCOutputStruct->TalkerAliasBlock3Data);
      }
      else
      {
        /* Store the Full Link Control Opcode (FLCO)  */
        FullLCOutputStruct->FullLinkControlOpcode = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[2], 6);

        fprintf(stderr, " FLCO/CSBKO = %d ", FullLCOutputStruct->FullLinkControlOpcode);
      }
      break;
    }

    /* BS_Dwn_Act PDU content - See ETSI TS 102 361-2 §7.1.2.1 BS Outbound Activation CSBK PDU */
    case 0b111000:
    {
      /* Store the Last Block (LB) bit */
      FullLCOutputStruct->LastBlock = (unsigned int)(InputLCDataBit[0] & 1);

      /* Store the Protect Flag (PF) bit */
      FullLCOutputStruct->ProtectFlag = (unsigned int)(InputLCDataBit[1] & 1);

      /* Store the Full Link CSBK Opcode (CSBKO)  */
      FullLCOutputStruct->FullLinkCSBKOpcode = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[2], 6);

      /* Store the Feature set ID (FID)  */
      FullLCOutputStruct->FeatureSetID = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[8], 8);

      /* Store the Service Options  */
      FullLCOutputStruct->Reserved = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[16], 16);

      /* Store the Group address (Talk Group) */
      FullLCOutputStruct->GroupAddress = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[32], 24);

      /* Store the Source address */
      FullLCOutputStruct->SourceAddress = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[56], 24);

      /* Print the destination ID (TG) and the source ID */
      fprintf(stderr, "| TG=%u  Src=%u  ", FullLCOutputStruct->GroupAddress, FullLCOutputStruct->SourceAddress);
      break;
    }

    /* Cap+ Rest Channel */
    case 0b111110:
    {
      /* Store the Last Block (LB) bit */
      FullLCOutputStruct->LastBlock = (unsigned int)(InputLCDataBit[0] & 1);

      /* Store the Protect Flag (PF) bit */
      FullLCOutputStruct->ProtectFlag = (unsigned int)(InputLCDataBit[1] & 1);

      /* Store the Full Link CSBK Opcode (CSBKO)  */
      FullLCOutputStruct->FullLinkCSBKOpcode = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[2], 6);

      /* Store the Feature set ID (FID)  */
      FullLCOutputStruct->FeatureSetID = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[8], 8);

      /* Store the number of available channel (RestChannel) */
      FullLCOutputStruct->RestChannel = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[20], 4);

      /* Store the channel used (ActiveChannel1) */
      FullLCOutputStruct->ActiveChannel1 = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[24], 8);

      /* Store the Group address (Talk Group) */
      FullLCOutputStruct->GroupAddress = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[32], 8);

      /* Store the Group address CSBK n°1 (Talk Group used in others channels) */
      FullLCOutputStruct->GroupAddressCSBK[0] = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[32], 8);

      /* Store the Group address CSBK n°2 (Talk Group used in others channels) */
      FullLCOutputStruct->GroupAddressCSBK[1] = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[40], 8);

      /* Store the Group address CSBK n°3 (Talk Group used in others channels) */
      FullLCOutputStruct->GroupAddressCSBK[2] = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[48], 8);

      /* Store the Group address CSBK n°4 (Talk Group used in others channels) */
      FullLCOutputStruct->GroupAddressCSBK[3] = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[56], 8);

      /* Store the Group address CSBK n°5 (Talk Group used in others channels) */
      FullLCOutputStruct->GroupAddressCSBK[4] = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[64], 8);

      /* Store the Group address CSBK n°6 (Talk Group used in others channels) */
      FullLCOutputStruct->GroupAddressCSBK[5] = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[72], 8);

      /* Set the Cap+ flag */
      state->CapacityPlusFlag = 1;

      /* Display the number of available channels */
      fprintf(stderr, "| Cap+ RestCh=%02u  ", FullLCOutputStruct->RestChannel);

      Offset = 0;
      BitToCheck = (1 << 7); /* 0x80 = 1st channel ; 0x40 = 2nd channel ; 0x20 = 3rd channel ... 0x02 = 7th channel ; 0x01 = 8th channel */
      for(i = 0; i < 8; i++)
      {
        /* Isolate 1 bit to know if there is a voice communication
         * in progress on the channel designed by this bit */
        if(FullLCOutputStruct->ActiveChannel1 & BitToCheck)
        {
          fprintf(stderr, "ActiveCh=%02u:", i + 1);
          fprintf(stderr, "TG=%u  ", FullLCOutputStruct->GroupAddressCSBK[Offset]);
          Offset++;

          /* A Cap+ CSBK frame may contains up to 6 TalkGroups, so prevent any overflow */
          if(Offset >= 6) Offset = 5;
        }

        /* Set the next channel used bit to check */
        BitToCheck >>= 1;
      }
      fprintf(stderr, "| ");

      //for(i = 0; i < 12; i++) LCDataBytes[i] = (char)ConvertBitIntoBytes(&InputLCDataBit[i * 8], 8);

      //fprintf(stderr, " | Data=0x%02X", LCDataBytes[2] & 0xFF);
      //for(i = 0; i < 10; i++)
      //{
      //  fprintf(stderr, "-0x%02X", LCDataBytes[i] & 0xFF);
      //}
      //fprintf(stderr, " ");

      break;
    }

    /* Cap+ Sites Neighbors */
    case 0b111011:
    {
      /* Store the Last Block (LB) bit */
      FullLCOutputStruct->LastBlock = (unsigned int)(InputLCDataBit[0] & 1);

      /* Store the Protect Flag (PF) bit */
      FullLCOutputStruct->ProtectFlag = (unsigned int)(InputLCDataBit[1] & 1);

      /* Store the Full Link CSBK Opcode (CSBKO)  */
      FullLCOutputStruct->FullLinkCSBKOpcode = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[2], 6);

      /* Store the Feature set ID (FID)  */
      FullLCOutputStruct->FeatureSetID = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[8], 8);

      /* Store the number of available channel (RestChannel) */
      FullLCOutputStruct->RestChannel = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[20], 4);

      /* Set the Cap+ flag */
      state->CapacityPlusFlag = 1;

      /* Display the number of available channels */
      fprintf(stderr, "| Cap+ RestCh=%02u  ", FullLCOutputStruct->RestChannel);

      /* Display the number of neighbors */
      fprintf(stderr, "Neighbors= ");

      for(i = 0; i < 6; i++)
      {
        FullLCOutputStruct->NeighborsSites[i] = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[(i * 8) + 32], 4);
        if(FullLCOutputStruct->NeighborsSites[i]) fprintf(stderr, "%u ", FullLCOutputStruct->NeighborsSites[i]);
      }

      fprintf(stderr, "| ");

      /* Convert all bit received to bytes */
      for(i = 0; i < 12; i++) LCDataBytes[i] = (char)ConvertBitIntoBytes(&InputLCDataBit[i * 8], 8);

      //fprintf(stderr, " [LB=%u  PF=%u  CSBKO=0x%02X  FID=0x%02X  DATA=%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X  CRC=0x%02X%02X] | ",
      //        FullLCOutputStruct->LastBlock, FullLCOutputStruct->ProtectFlag, FullLCOutputStruct->FullLinkCSBKOpcode & 0b111111,
      //        FullLCOutputStruct->FeatureSetID,
      //        LCDataBytes[2]  & 0xFF, LCDataBytes[3]  & 0xFF, LCDataBytes[4] & 0xFF, LCDataBytes[5] & 0xFF,
      //        LCDataBytes[6]  & 0xFF, LCDataBytes[7]  & 0xFF, LCDataBytes[8] & 0xFF, LCDataBytes[9] & 0xFF,
      //        LCDataBytes[10] & 0xFF, LCDataBytes[11] & 0xFF);

      //fprintf(stderr, "| Data=0x%02X", LCDataBytes[0] & 0xFF);
      //for(i = 1; i < 10; i++)
      //{
      //  fprintf(stderr, "-0x%02X", LCDataBytes[i] & 0xFF);
      //}
      //fprintf(stderr, " ");

      break;
    }

    default:
    {
      /* Store the Feature set ID (FID)  */
      FullLCOutputStruct->FeatureSetID = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[8], 8);

      /* Convert received bits into bytes */
      for(i = 0; i < 12; i++) LCDataBytes[i] = (char)ConvertBitIntoBytes(&InputLCDataBit[i * 8], 8);

      if(CSBKContent)
      {
        /* Store the Last Block (LB) bit */
        FullLCOutputStruct->LastBlock = (unsigned int)(InputLCDataBit[0] & 1);

        /* Store the Protect Flag (PF) bit */
        FullLCOutputStruct->ProtectFlag = (unsigned int)(InputLCDataBit[1] & 1);

        /* Store the Full Link CSBK Opcode (CSBKO)  */
        FullLCOutputStruct->FullLinkCSBKOpcode = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[2], 6);

        fprintf(stderr, "| Unknown CSBKO  [LB=%u  PF=%u  CSBKO=0x%02X  FID=0x%02X  DATA=%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X  CRC=0x%02X%02X] | ",
                FullLCOutputStruct->LastBlock, FullLCOutputStruct->ProtectFlag, FullLCOutputStruct->FullLinkCSBKOpcode & 0b111111,
                FullLCOutputStruct->FeatureSetID,
                LCDataBytes[2]  & 0xFF, LCDataBytes[3]  & 0xFF, LCDataBytes[4] & 0xFF, LCDataBytes[5] & 0xFF,
                LCDataBytes[6]  & 0xFF, LCDataBytes[7]  & 0xFF, LCDataBytes[8] & 0xFF, LCDataBytes[9] & 0xFF,
                LCDataBytes[10] & 0xFF, LCDataBytes[11] & 0xFF);

        /* Print unknown CSBK content = 80 bits */
        //fprintf(stderr, "Data=0x%02X", LCDataBytes[0] & 0xFF);
        //for(i = 1; i < 12; i++)
        //{
        //  fprintf(stderr, "-0x%02X", LCDataBytes[i] & 0xFF);
        //}
        //fprintf(stderr, " ");
      }
      else
      {
        /* Store the Protect Flag (PF) bit */
        FullLCOutputStruct->ProtectFlag = (unsigned int)(InputLCDataBit[0] & 1);

        /* Store the Reserved bit */
        FullLCOutputStruct->Reserved = (unsigned int)(InputLCDataBit[1] & 1);

        /* Store the Full Link Control Opcode (FLCO)  */
        FullLCOutputStruct->FullLinkControlOpcode = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[2], 6);

        /* Store the Feature set ID (FID)  */
        FullLCOutputStruct->FeatureSetID = (unsigned int)ConvertBitIntoBytes(&InputLCDataBit[8], 8);

        if(VoiceCallInProgress)
        {
          fprintf(stderr, "| Unknown FLCO  [PF=%u  R=%u  FLCO=0x%02X  FID=0x%02X  DATA=%02X-%02X-%02X-%02X-%02X-%02X-%02X  CRC=0x%02X] | ",
                  FullLCOutputStruct->ProtectFlag, FullLCOutputStruct->Reserved, FullLCOutputStruct->FullLinkControlOpcode & 0b111111,
                  FullLCOutputStruct->FeatureSetID,
                  LCDataBytes[2] & 0xFF, LCDataBytes[3]  & 0xFF, LCDataBytes[4] & 0xFF, LCDataBytes[5] & 0xFF,
                  LCDataBytes[6] & 0xFF, LCDataBytes[7]  & 0xFF, LCDataBytes[8] & 0xFF,
                  (LCDataBytes[9] >> 3) & 0x1F);
        }
        else
        {
          fprintf(stderr, "| Unknown FLCO  [PF=%u  R=%u  FLCO=0x%02X  FID=0x%02X  DATA=%02X-%02X-%02X-%02X-%02X-%02X-%02X  CRC=0x%02X%02X%02X] | ",
                  FullLCOutputStruct->ProtectFlag, FullLCOutputStruct->Reserved, FullLCOutputStruct->FullLinkControlOpcode & 0b111111,
                  FullLCOutputStruct->FeatureSetID,
                  LCDataBytes[2] & 0xFF, LCDataBytes[3]  & 0xFF, LCDataBytes[4] & 0xFF, LCDataBytes[5] & 0xFF,
                  LCDataBytes[6] & 0xFF, LCDataBytes[7]  & 0xFF, LCDataBytes[8] & 0xFF,
                  LCDataBytes[9] & 0xFF, LCDataBytes[10] & 0xFF, LCDataBytes[11] & 0xFF);
        }

        //fprintf(stderr, "Data=0x%02X", LCDataBytes[0] & 0xFF);
        //for(i = 1; i < 11; i++)
        //{
        //  fprintf(stderr, "-0x%02X", LCDataBytes[i] & 0xFF);
        //}
        //fprintf(stderr, " ");
      }
      break;
    }
  } /* End switch(FullLCOutputStruct->FullLinkControlOpcode & 0b111111) */

} /* End DmrFullLinkControlDecode() */


/*
 * @brief : This function compute the GPS position from the full link control GPS data
 *          See ETSI TS 102 361-2
 *          - §7.2.15 Position Error
 *          - §7.2.16 Longitude
 *          - §7.2.17 Latitude
 *
 * @param GPSLongitude : The GPS longitude (25 bits received from LC data)
 *                       360/2^25 degrees format
 *                       Range -180 degrees to +(180 - 360/2^25) degrees
 *                       Negative values = West - Positives values = East
 *                       See §7.2.16 Longitude
 *
 * @param GPSLatitude : The GPS latitude (24 bits received from LC data)
 *                      180/2^24 degrees format
 *                      Range -90 degrees to +(90 - 180/2^24) degrees
 *                      Negative values = South - Positives values = North
 *                      See §7.2.17 Latitude
 *
 * @param GPSPositionError : The GPS position error (3 bits received from LC data)
 *                           See §7.2.15 Position Error
 *
 * @return None
 */
void PrintDmrGpsPositionFromLinkControlData(unsigned int GPSLongitude,
                                            unsigned int GPSLatitude,
                                            unsigned int GPSPositionError)
{
  int GPSLongitudeInput = (int)GPSLongitude;
  int GPSLatitudeInput = (int)GPSLatitude;
  double Longitude = 0.0;
  double Latitude = 0.0;
  double NegativeLongitude = 1.0;
  double NegativeLatitude = 1.0;
  char   LongitudeEastWest[16] = "East";
  char   LatitudeNorthSouth[16] = "North";
  char   PositionErrorDistance[32] = {0};

  /* Check if the longitude is negative (check 25th bit - The MSBit received in LC data) */
  if(GPSLongitude & 0x01000000)
  {
    NegativeLongitude = -1.0;
    strcpy(LongitudeEastWest, "West");
    GPSLongitudeInput |= 0xFE000000; /* Enable all MSBit to '1' to consider the longitude as a negative number */
    GPSLongitudeInput = (~GPSLongitudeInput + 1) & 0x01FFFFFF;
  }

  /* Check if the latitude is negative (check 24th bit - The MSBit received in LC data) */
  if(GPSLatitude & 0x00800000)
  {
    NegativeLatitude = -1.0;
    strcpy(LatitudeNorthSouth, "South");
    GPSLatitudeInput |= 0xFF000000; /* Enable all MSBit to '1' to consider the latitude as a negative number */
    GPSLatitudeInput = (~GPSLatitudeInput + 1) & 0x00FFFFFF;
  }

  /* Compute longitude */
  Longitude = (double)((double)GPSLongitudeInput * 360.0 / (double)(1 << 25)) * NegativeLongitude;

  /* Compute latitude */
  Latitude = (double)((double)GPSLatitudeInput * 180.0 / (double)(1 << 24)) * NegativeLatitude;

  /* Compute GPS position error (3 bits from LC data)
   * See §7.2.15 Position Error */
  switch(GPSPositionError & 0x07)
  {
    case 0x00:
    {
      strcpy(PositionErrorDistance, "Less than 2 m");
      break;
    }

    case 0x01:
    {
      strcpy(PositionErrorDistance, "Less than 20 m");
      break;
    }

    case 0x02:
    {
      strcpy(PositionErrorDistance, "Less than 200 m");
      break;
    }

    case 0x03:
    {
      strcpy(PositionErrorDistance, "Less than 2 km");
      break;
    }

    case 0x04:
    {
      strcpy(PositionErrorDistance, "Less than 20 km");
      break;
    }

    case 0x05:
    {
      strcpy(PositionErrorDistance, "Less than or equal to 200 km");
      break;
    }

    case 0x06:
    {
      strcpy(PositionErrorDistance, "More than 200 km");
      break;
    }

    case 0x07:
    {
      strcpy(PositionErrorDistance, "Unknown or error");
      break;
    }

    /* We should ever go here */
    default:
    {
      strcpy(PositionErrorDistance, "Unknown or error");
      break;
    }
  }

  fprintf(stderr, "GPS - Longitude : %f %s - Latitude : %f %s - GPS Precision %s ",
          Longitude, LongitudeEastWest, Latitude, LatitudeNorthSouth, PositionErrorDistance);
} /* End PrintDmrGpsPositionFromLinkControlData() */


/*
 * @brief : This function extract the talker alias from the full link control talker alias data
 *          See ETSI TS 102 361-2
 *          - §7.1.1.4 Talker Alias header LC PDU
 *          - §7.1.1.5 Talker Alias block LC PDU
 *          - §7.2.18 Talker Alias Data Format
 *          - §7.2.19 Talker Alias Data Length
 *
 * @param TalkerAliasHeaderDataFormat : 2 bits of data format
 *   @arg 00 : 7 bits character
 *   @arg 01 : ISO 8 bits character
 *   @arg 10 : Unicode UTF-8
 *   @arg 11 : Unicode UTF-16BE
 *
 * @param TalkerAliasHeaderDataLength
 *
 * @param TalkerAliasHeaderData : 49 bits data of the header block
 *
 * @param TalkerAliasBlock1Data : 56 bits data of the 1st block
 *
 * @param TalkerAliasBlock2Data : 56 bits data of the 2nd block
 *
 * @param TalkerAliasBlock3Data : 56 bits data of the 3rd block
 *
 * @return None
 *
 */
void PrintDmrTalkerAliasFromLinkControlData(unsigned int  TalkerAliasHeaderDataFormat,
                                            unsigned int  TalkerAliasHeaderDataLength,
                                            unsigned int  TalkerAliasBlockNumber,
                                            unsigned char TalkerAliasHeaderData[49],
                                            unsigned char TalkerAliasBlock1Data[56],
                                            unsigned char TalkerAliasBlock2Data[56],
                                            unsigned char TalkerAliasBlock3Data[56])
{
  int ReadSize = 0;
  int ReadOffset = 0;
  int NumberOfByteRead = 0;
  unsigned int NumberOfBlockToReceive = 0;
  char CharacterFormat[32] = {0};
  char TalkerAliasCompleteData[217]; /* Complete block data (49 + 56 + 56 + 56 bits = 217 bits) */
  char FinalTalkerAlias[32] = {0};
  int i, j;
  char Temp;

  if((TalkerAliasHeaderData == NULL) || (TalkerAliasBlock1Data == NULL) ||
     (TalkerAliasBlock2Data == NULL) || (TalkerAliasBlock3Data == NULL)) return;

  /* Check the data format */
  switch(TalkerAliasHeaderDataFormat & 0x03)
  {
    case 0b00:
    {
      ReadSize = 7;
      ReadOffset = 0;
      sprintf(CharacterFormat, "7 bits character");
      break;
    }

    case 0b01:
    {
      ReadSize = 8;
      ReadOffset = 1;
      sprintf(CharacterFormat, "ISO 8 bit character");
      break;
    }

    case 0b10:
    {
      ReadSize = 8;
      ReadOffset = 1;
      sprintf(CharacterFormat, "Unicode UTF-8");
      break;
    }

    case 0b11:
    {
      ReadSize = 8;
      ReadOffset = 1;
      sprintf(CharacterFormat, "Unicode UTF-16BE");
      break;
    }

    default:
    {
      ReadSize = 7;
      ReadOffset = 0;
      sprintf(CharacterFormat, "Unknown character");
      break;
    }
  }

  /* Reconstitutes the complete talker alias message (in bits format) */
  for(i = 0, j = 0; i < 49; i++, j++) TalkerAliasCompleteData[j] = TalkerAliasHeaderData[i];
  for(i = 0; i < 56; i++, j++) TalkerAliasCompleteData[j] = TalkerAliasBlock1Data[i];
  for(i = 0; i < 56; i++, j++) TalkerAliasCompleteData[j] = TalkerAliasBlock2Data[i];
  for(i = 0; i < 56; i++, j++) TalkerAliasCompleteData[j] = TalkerAliasBlock3Data[i];

  /* Reconstitutes the complete talker alias message (in bytes/characters format) */
  for(i = 0, NumberOfByteRead = ReadOffset; i < (int)(TalkerAliasHeaderDataLength & 0x1F); i++)
  {
    /* Convert the 7 or 8 bits field into characters */
    for(j = 0, Temp = 0; j < ReadSize; j++)
    {
      Temp  = Temp << 1;
      Temp |= (TalkerAliasCompleteData[NumberOfByteRead++] & 1);
    }

    /* Store the 7 or 8 bit character */
    FinalTalkerAlias[i] = Temp;
  }

  /* Check the number of block to receive */
  if(NumberOfByteRead <= 49) NumberOfBlockToReceive = 0;
  else if((NumberOfByteRead > 49) && (NumberOfByteRead <= (49 + 56))) NumberOfBlockToReceive = 1;
  else if((NumberOfByteRead > (49 + 56)) && (NumberOfByteRead <= (49 + 56 + 56))) NumberOfBlockToReceive = 2;
  else NumberOfBlockToReceive = 3;

  /* Display the alias only when all block have been received */
  if(NumberOfBlockToReceive == TalkerAliasBlockNumber)
  {
    fprintf(stderr, "| Length=%u - Value=\"", TalkerAliasHeaderDataLength);

    for(i = 0; i < (int)(TalkerAliasHeaderDataLength & 0x1F); i++)
    {
      fprintf(stderr, "%c", FinalTalkerAlias[i] & 0xFF);
    }

    fprintf(stderr, "\" - Format %s ", CharacterFormat);
  }

} /* End PrintDmrTalkerAliasFromLinkControlData() */


void DmrBackupLinkControlData(dsd_opts * opts, dsd_state * state)
{
  UNUSED_VARIABLE(opts);

  /* Backup only Link Control data that use several block */
  /* Talker alias Link Control uses several blocks (Time Slot 1 backup) */
  g_TS1_TalkerAliasHeaderDataFormat = state->TS1SuperFrame.FullLC.TalkerAliasHeaderDataFormat;
  g_TS1_TalkerAliasHeaderDataLength = state->TS1SuperFrame.FullLC.TalkerAliasHeaderDataLength;
  memcpy(g_TS1_TalkerAliasHeaderData, state->TS1SuperFrame.FullLC.TalkerAliasHeaderData, sizeof(g_TS1_TalkerAliasHeaderData));
  memcpy(g_TS1_TalkerAliasBlock1Data, state->TS1SuperFrame.FullLC.TalkerAliasBlock1Data, sizeof(g_TS1_TalkerAliasBlock1Data));
  memcpy(g_TS1_TalkerAliasBlock2Data, state->TS1SuperFrame.FullLC.TalkerAliasBlock2Data, sizeof(g_TS1_TalkerAliasBlock2Data));
  memcpy(g_TS1_TalkerAliasBlock3Data, state->TS1SuperFrame.FullLC.TalkerAliasBlock3Data, sizeof(g_TS1_TalkerAliasBlock3Data));

  /* Talker alias Link Control uses several blocks (Time Slot 2 backup) */
  g_TS2_TalkerAliasHeaderDataFormat = state->TS2SuperFrame.FullLC.TalkerAliasHeaderDataFormat;
  g_TS2_TalkerAliasHeaderDataLength = state->TS2SuperFrame.FullLC.TalkerAliasHeaderDataLength;
  memcpy(g_TS2_TalkerAliasHeaderData, state->TS2SuperFrame.FullLC.TalkerAliasHeaderData, sizeof(g_TS2_TalkerAliasHeaderData));
  memcpy(g_TS2_TalkerAliasBlock1Data, state->TS2SuperFrame.FullLC.TalkerAliasBlock1Data, sizeof(g_TS2_TalkerAliasBlock1Data));
  memcpy(g_TS2_TalkerAliasBlock2Data, state->TS2SuperFrame.FullLC.TalkerAliasBlock2Data, sizeof(g_TS2_TalkerAliasBlock2Data));
  memcpy(g_TS2_TalkerAliasBlock3Data, state->TS2SuperFrame.FullLC.TalkerAliasBlock3Data, sizeof(g_TS2_TalkerAliasBlock3Data));
} /* End BackupLinkControlData() */


void DmrRestoreLinkControlData(dsd_opts * opts, dsd_state * state)
{
  UNUSED_VARIABLE(opts);

  /* Talker alias Link Control uses several blocks (Time Slot 1 backup) */
  state->TS1SuperFrame.FullLC.TalkerAliasHeaderDataFormat = g_TS1_TalkerAliasHeaderDataFormat;
  state->TS1SuperFrame.FullLC.TalkerAliasHeaderDataLength = g_TS1_TalkerAliasHeaderDataLength;
  memcpy(state->TS1SuperFrame.FullLC.TalkerAliasHeaderData, g_TS1_TalkerAliasHeaderData, sizeof(g_TS1_TalkerAliasHeaderData));
  memcpy(state->TS1SuperFrame.FullLC.TalkerAliasBlock1Data, g_TS1_TalkerAliasBlock1Data, sizeof(g_TS1_TalkerAliasBlock1Data));
  memcpy(state->TS1SuperFrame.FullLC.TalkerAliasBlock2Data, g_TS1_TalkerAliasBlock2Data, sizeof(g_TS1_TalkerAliasBlock2Data));
  memcpy(state->TS1SuperFrame.FullLC.TalkerAliasBlock3Data, g_TS1_TalkerAliasBlock3Data, sizeof(g_TS1_TalkerAliasBlock3Data));

  /* Talker alias Link Control uses several blocks (Time Slot 2 backup) */
  state->TS2SuperFrame.FullLC.TalkerAliasHeaderDataFormat = g_TS2_TalkerAliasHeaderDataFormat;
  state->TS2SuperFrame.FullLC.TalkerAliasHeaderDataLength = g_TS2_TalkerAliasHeaderDataLength;
  memcpy(state->TS2SuperFrame.FullLC.TalkerAliasHeaderData, g_TS2_TalkerAliasHeaderData, sizeof(g_TS2_TalkerAliasHeaderData));
  memcpy(state->TS2SuperFrame.FullLC.TalkerAliasBlock1Data, g_TS2_TalkerAliasBlock1Data, sizeof(g_TS2_TalkerAliasBlock1Data));
  memcpy(state->TS2SuperFrame.FullLC.TalkerAliasBlock2Data, g_TS2_TalkerAliasBlock2Data, sizeof(g_TS2_TalkerAliasBlock2Data));
  memcpy(state->TS2SuperFrame.FullLC.TalkerAliasBlock3Data, g_TS2_TalkerAliasBlock3Data, sizeof(g_TS2_TalkerAliasBlock3Data));
}


/* End of file */
