/*
 ============================================================================
 Name        : dmr_data_content_decode.c
 Author      : Louis-Erig HERVE - F4HUZ
 Version     : V1.0
 Date        : 2022 October 16th
 Copyright   : None
 Description : DMR data management library
 ============================================================================
 */

/* Include ------------------------------------------------------------------*/
#include "dsd.h"


/* Global variables ---------------------------------------------------------*/


/* Functions ----------------------------------------------------------------*/

/*
 * @brief : This function initialize the library witch manages the received DMR data
 *
 * @param None
 *
 * @return None
 */
void DmrDataContentInitLib(void)
{

} /* End DmrDataContentInitLib() */


/*
 * @brief : This function decodes the full link control data (56 bits)
 *          See ETSI TS 102 361-1 §9.1.6 Full Link Control (FULL LC) PDU.
 *
 * @param InputDataBit : The 96 bit (carried as 96 bytes) of the data header
 *
 * @param DMRDataStruct : A structure pointer to store all parameters
 *
 * @return None
 */
void DmrDataHeaderDecode(uint8_t InputDataBit[96], DMRDataPDU_t * DMRDataStruct)
{
  uint32_t i;
  char DataHeaderBytes[12];

  /* Store the 96 bits [80 dta bits + 16 bits CRC = 96 bits] of the data header */
  for(i = 0; i < 96; i++)
  {
    DMRDataStruct->DataHeaderBit[i] = (char)InputDataBit[i] & 1;
  }

  /* Store the Data Packet Format */
  DMRDataStruct->DataPacketFormat = (int)ConvertBitIntoBytes(&InputDataBit[4], 4);

  /* Store the Destination Logical Link ID */
  DMRDataStruct->DestinationLogicalLinkID = (int)ConvertBitIntoBytes(&InputDataBit[16], 24);

  /* Store the Source Logical Link ID */
  DMRDataStruct->SourceLogicalLinkID = (int)ConvertBitIntoBytes(&InputDataBit[40], 24);

  switch(DMRDataStruct->DataPacketFormat & 0b1111)
  {
    /* Unified Data Transport (UDT) */
    case 0b0000:
    {
      /* Store the group/individual call bit */
      DMRDataStruct->GroupDataCall = (int)InputDataBit[0] & 1;

      /* Store the response requested bit */
      DMRDataStruct->ResponseRequested = (int)InputDataBit[1] & 1;

      /* Store the Data Packet Format */
      DMRDataStruct->DataPacketFormat = (int)ConvertBitIntoBytes(&InputDataBit[4], 4);

      /* Store the SAP Identifier */
      DMRDataStruct->SAPIdentifier = (int)ConvertBitIntoBytes(&InputDataBit[8], 4);

      /* Store the Destination Logical Link ID (LLID) */
      DMRDataStruct->DestinationLogicalLinkID = (int)ConvertBitIntoBytes(&InputDataBit[16], 24);

      /* Store the Source Logical Link ID (LLID) */
      DMRDataStruct->SourceLogicalLinkID = (int)ConvertBitIntoBytes(&InputDataBit[40], 24);

      /* Store the Pad Nibble */
      DMRDataStruct->PadNibble = (int)ConvertBitIntoBytes(&InputDataBit[64], 5);

      /* Store the number of blocks to follow */
      DMRDataStruct->BlocksToFollow = (int)ConvertBitIntoBytes(&InputDataBit[70], 2);

      /* Store the Supplementary Flag (SF) */
      DMRDataStruct->SupplementaryFlag = (int)InputDataBit[72] & 1;

      /* Store the Supplementary Flag (SF) */
      DMRDataStruct->ProtectFlag = (int)InputDataBit[73] & 1;

      /* Store the Unified Data Transport Opcode (UDTO) */
      DMRDataStruct->UDTOpcode = (int)ConvertBitIntoBytes(&InputDataBit[74], 6);

      fprintf(stderr, "| Unified Data Header  [%s  BlocksNb=%u  Dst=%u  Src=%u  A=%u  UDTFormat=%u  PadNibble=%u  SF=%u  PF=%u  UDTO=%u  SAP=%u (%s)] ",
              DMRDataStruct->GroupDataCall ? "GRP":"IND",
              DMRDataStruct->BlocksToFollow,
              DMRDataStruct->DestinationLogicalLinkID,
              DMRDataStruct->SourceLogicalLinkID,
              DMRDataStruct->ResponseRequested,
              DMRDataStruct->UDTFormat,
              DMRDataStruct->PadNibble,
              DMRDataStruct->SupplementaryFlag,
              DMRDataStruct->ProtectFlag,
              DMRDataStruct->UDTOpcode,
              DMRDataStruct->SAPIdentifier,
              DmrDataServiceAccessPointIdentifierToStr(DMRDataStruct->SAPIdentifier));
      break;
    }

    /* Response packet */
    case 0b0001:
    {
      /* Store the group/individual call bit */
      DMRDataStruct->GroupDataCall = (int)InputDataBit[0] & 1;

      /* Store the response requested bit */
      DMRDataStruct->ResponseRequested = (int)InputDataBit[1] & 1;

      /* Store the Data Packet Format */
      DMRDataStruct->DataPacketFormat = (int)ConvertBitIntoBytes(&InputDataBit[4], 4);

      /* Store the SAP Identifier */
      DMRDataStruct->SAPIdentifier = (int)ConvertBitIntoBytes(&InputDataBit[8], 4);

      /* Store the Destination Logical Link ID (LLID) */
      DMRDataStruct->DestinationLogicalLinkID = (int)ConvertBitIntoBytes(&InputDataBit[16], 24);

      /* Store the Source Logical Link ID (LLID) */
      DMRDataStruct->SourceLogicalLinkID = (int)ConvertBitIntoBytes(&InputDataBit[40], 24);

      /* Store the number of blocks to follow */
      DMRDataStruct->BlocksToFollow = (int)ConvertBitIntoBytes(&InputDataBit[65], 7);

      /* Store the Class */
      DMRDataStruct->Class = (int)ConvertBitIntoBytes(&InputDataBit[72], 2);

      /* Store the Type */
      DMRDataStruct->Type = (int)ConvertBitIntoBytes(&InputDataBit[74], 3);

      /* Store the Status */
      DMRDataStruct->Status = (int)ConvertBitIntoBytes(&InputDataBit[77], 3);

      fprintf(stderr, "| Response Data Header  [%s  BlocksNb=%u  Dst=%u  Src=%u  A=%u  Class=%u  Type=%u  Status=%u  SAP=%u (%s)] ",
              DMRDataStruct->GroupDataCall ? "GRP":"IND",
              DMRDataStruct->BlocksToFollow,
              DMRDataStruct->DestinationLogicalLinkID,
              DMRDataStruct->SourceLogicalLinkID,
              DMRDataStruct->ResponseRequested,
              DMRDataStruct->Class,
              DMRDataStruct->Type,
              DMRDataStruct->Status,
              DMRDataStruct->SAPIdentifier,
              DmrDataServiceAccessPointIdentifierToStr(DMRDataStruct->SAPIdentifier));
      break;
    }

    /* Data packet with unconfirmed delivery */
    case 0b0010:
    {
      /* Store the group/individual call bit */
      DMRDataStruct->GroupDataCall = (int)InputDataBit[0] & 1;

      /* Store the response requested bit */
      DMRDataStruct->ResponseRequested = (int)InputDataBit[1] & 1;

      /* Store the Data Packet Format */
      DMRDataStruct->DataPacketFormat = (int)ConvertBitIntoBytes(&InputDataBit[4], 4);

      /* Store the SAP Identifier */
      DMRDataStruct->SAPIdentifier = (int)ConvertBitIntoBytes(&InputDataBit[8], 4);

      /* Store the padding octet number */
      DMRDataStruct->PadOctetCount = (int)ConvertBitIntoBytes(&InputDataBit[12], 4);
      DMRDataStruct->PadOctetCount |= (((int)InputDataBit[3] & 1) << 4);

      /* Store the Destination Logical Link ID (LLID) */
      DMRDataStruct->DestinationLogicalLinkID = (int)ConvertBitIntoBytes(&InputDataBit[16], 24);

      /* Store the Source Logical Link ID (LLID) */
      DMRDataStruct->SourceLogicalLinkID = (int)ConvertBitIntoBytes(&InputDataBit[40], 24);

      /* Store the Full Message Flag bit */
      DMRDataStruct->FullMessageFlag = (int)InputDataBit[64] & 1;

      /* Store the number of blocks to follow */
      DMRDataStruct->BlocksToFollow = (int)ConvertBitIntoBytes(&InputDataBit[65], 7);

      /* Store the fragment sequence number (FSN) */
      DMRDataStruct->FragmentSequenceNumber = (int)ConvertBitIntoBytes(&InputDataBit[76], 4);

      fprintf(stderr, "| Unconfirmed Data Header  [%s  PadBytes=%02u  BlocksNb=%u  Dst=%u  Src=%u  A=%u  F=%u  FSN=%02u  SAP=%u (%s)] ",
              DMRDataStruct->GroupDataCall ? "GRP":"IND",
              DMRDataStruct->PadOctetCount,
              DMRDataStruct->BlocksToFollow,
              DMRDataStruct->DestinationLogicalLinkID,
              DMRDataStruct->SourceLogicalLinkID,
              DMRDataStruct->ResponseRequested,
              DMRDataStruct->FullMessageFlag,
              DMRDataStruct->FragmentSequenceNumber,
              DMRDataStruct->SAPIdentifier,
              DmrDataServiceAccessPointIdentifierToStr(DMRDataStruct->SAPIdentifier));
      break;
    }

    /* Data packet with confirmed delivery */
    case 0b0011:
    {
      /* Store the group/individual call bit */
      DMRDataStruct->GroupDataCall = (int)InputDataBit[0] & 1;

      /* Store the response requested bit */
      DMRDataStruct->ResponseRequested = (int)InputDataBit[1] & 1;

      /* Store the Data Packet Format */
      DMRDataStruct->DataPacketFormat = (int)ConvertBitIntoBytes(&InputDataBit[4], 4);

      /* Store the SAP Identifier */
      DMRDataStruct->SAPIdentifier = (int)ConvertBitIntoBytes(&InputDataBit[8], 4);

      /* Store the padding octet number */
      DMRDataStruct->PadOctetCount = (int)ConvertBitIntoBytes(&InputDataBit[12], 4);
      DMRDataStruct->PadOctetCount |= (((int)InputDataBit[3] & 1) << 4);

      /* Store the Destination Logical Link ID (LLID) */
      DMRDataStruct->DestinationLogicalLinkID = (int)ConvertBitIntoBytes(&InputDataBit[16], 24);

      /* Store the Source Logical Link ID (LLID) */
      DMRDataStruct->SourceLogicalLinkID = (int)ConvertBitIntoBytes(&InputDataBit[40], 24);

      /* Store the Full Message Flag bit */
      DMRDataStruct->FullMessageFlag = (int)InputDataBit[64] & 1;

      /* Store the number of blocks to follow */
      DMRDataStruct->BlocksToFollow = (int)ConvertBitIntoBytes(&InputDataBit[65], 7);

      /* Store the Re-Synchronize Flag bit */
      DMRDataStruct->ReSyncronizeFlag = (int)InputDataBit[72] & 1;

      /* Store the Send Sequence Number (N(S)) */
      DMRDataStruct->SendSequenceNumber = (int)ConvertBitIntoBytes(&InputDataBit[73], 3);

      /* Store the fragment sequence number (FSN) */
      DMRDataStruct->FragmentSequenceNumber = (int)ConvertBitIntoBytes(&InputDataBit[76], 4);

      fprintf(stderr, "| Confirmed Data Header  [%s  PadBytes=%02u  BlocksNb=%u  Dst=%u  Src=%u  A=%u  F=%u  FSN=%02u  N(S)=%u  SAP=%u (%s)] ",
              DMRDataStruct->GroupDataCall ? "GRP":"IND",
              DMRDataStruct->PadOctetCount,
              DMRDataStruct->BlocksToFollow,
              DMRDataStruct->DestinationLogicalLinkID,
              DMRDataStruct->SourceLogicalLinkID,
              DMRDataStruct->ResponseRequested,
              DMRDataStruct->FullMessageFlag,
              DMRDataStruct->FragmentSequenceNumber,
              DMRDataStruct->SendSequenceNumber,
              DMRDataStruct->SAPIdentifier,
              DmrDataServiceAccessPointIdentifierToStr(DMRDataStruct->SAPIdentifier));
      break;
    }

    /* Short Data: Defined */
    case 0b1101:
    {
      /* Store the group/individual call bit */
      DMRDataStruct->GroupDataCall = (int)InputDataBit[0] & 1;

      /* Store the response requested bit */
      DMRDataStruct->ResponseRequested = (int)InputDataBit[1] & 1;

      /* Store the Appended Blocks */
      DMRDataStruct->AppendedBlocks  = ((int)InputDataBit[2]  & 1) << 5;
      DMRDataStruct->AppendedBlocks |= ((int)InputDataBit[3]  & 1) << 4;
      DMRDataStruct->AppendedBlocks |= ((int)InputDataBit[12] & 1) << 3;
      DMRDataStruct->AppendedBlocks |= ((int)InputDataBit[13] & 1) << 2;
      DMRDataStruct->AppendedBlocks |= ((int)InputDataBit[14] & 1) << 1;
      DMRDataStruct->AppendedBlocks |= ((int)InputDataBit[15] & 1) << 0;

      /* Store the Data Packet Format */
      DMRDataStruct->DataPacketFormat = (int)ConvertBitIntoBytes(&InputDataBit[4], 4);

      /* Store the SAP Identifier */
      DMRDataStruct->SAPIdentifier = (int)ConvertBitIntoBytes(&InputDataBit[8], 4);

      /* Store the Destination Logical Link ID (LLID) */
      DMRDataStruct->DestinationLogicalLinkID = (int)ConvertBitIntoBytes(&InputDataBit[16], 24);

      /* Store the Source Logical Link ID (LLID) */
      DMRDataStruct->SourceLogicalLinkID = (int)ConvertBitIntoBytes(&InputDataBit[40], 24);

      /* Store the Status/Precoded (S/P) */
      DMRDataStruct->DefinedData = (int)ConvertBitIntoBytes(&InputDataBit[64], 6);

      /* Store the Selective Automatic Repeat ReQuest (SARQ) */
      DMRDataStruct->SelecriveAutomaticRepeatReQuest = (int)InputDataBit[70] & 1;

      /* Store the Full Message Flag bit */
      DMRDataStruct->FullMessageFlag = (int)InputDataBit[71] & 1;

      /* Store the Bit Padding */
      DMRDataStruct->BitPadding = (int)ConvertBitIntoBytes(&InputDataBit[72], 8);

      fprintf(stderr, "| Defined Short Data Header  [%s  Dst=%u  Src=%u  AB=%u  A=%u  DefinedData=%u  SARQ=%u  F=%u  SAP=%u (%s)] ",
                    DMRDataStruct->GroupDataCall ? "GRP":"IND",
                    DMRDataStruct->DestinationLogicalLinkID,
                    DMRDataStruct->SourceLogicalLinkID,
                    DMRDataStruct->AppendedBlocks,
                    DMRDataStruct->ResponseRequested,
                    DMRDataStruct->DefinedData,
                    DMRDataStruct->SelecriveAutomaticRepeatReQuest,
                    DMRDataStruct->FullMessageFlag,
                    DMRDataStruct->SAPIdentifier,
                    DmrDataServiceAccessPointIdentifierToStr(DMRDataStruct->SAPIdentifier));
      break;
    }

    /* Short Data: Raw or Status/Precoded */
    case 0b1110:
    {
      /* Store the group/individual call bit */
      DMRDataStruct->GroupDataCall = (int)InputDataBit[0] & 1;

      /* Store the response requested bit */
      DMRDataStruct->ResponseRequested = (int)InputDataBit[1] & 1;

      /* Store the Appended Blocks */
      DMRDataStruct->AppendedBlocks  = ((int)InputDataBit[2]  & 1) << 5;
      DMRDataStruct->AppendedBlocks |= ((int)InputDataBit[3]  & 1) << 4;
      DMRDataStruct->AppendedBlocks |= ((int)InputDataBit[12] & 1) << 3;
      DMRDataStruct->AppendedBlocks |= ((int)InputDataBit[13] & 1) << 2;
      DMRDataStruct->AppendedBlocks |= ((int)InputDataBit[14] & 1) << 1;
      DMRDataStruct->AppendedBlocks |= ((int)InputDataBit[15] & 1) << 0;

      /* Store the Data Packet Format */
      DMRDataStruct->DataPacketFormat = (int)ConvertBitIntoBytes(&InputDataBit[4], 4);

      /* Store the SAP Identifier */
      DMRDataStruct->SAPIdentifier = (int)ConvertBitIntoBytes(&InputDataBit[8], 4);

      /* Store the Destination Logical Link ID (LLID) */
      DMRDataStruct->DestinationLogicalLinkID = (int)ConvertBitIntoBytes(&InputDataBit[16], 24);

      /* Store the Source Logical Link ID (LLID) */
      DMRDataStruct->SourceLogicalLinkID = (int)ConvertBitIntoBytes(&InputDataBit[40], 24);

      /* Store the Source Port (SP) */
      DMRDataStruct->SourcePort = (int)ConvertBitIntoBytes(&InputDataBit[64], 3);

      /* Store the Destination Port (DP) */
      DMRDataStruct->DestinationPort = (int)ConvertBitIntoBytes(&InputDataBit[67], 3);

      /* -------- Available only on status/precoded short data header -------- */
      /* Store the Status/Precoded (S/P) */
      DMRDataStruct->StatusPrecoded = (int)ConvertBitIntoBytes(&InputDataBit[70], 10);

      /* -------- Available only on raw short data packet header -------- */
      /* Store the Selective Automatic Repeat ReQuest (SARQ) */
      DMRDataStruct->SelecriveAutomaticRepeatReQuest = (int)InputDataBit[70] & 1;

      /* -------- Available only on raw short data packet header -------- */
      /* Store the Full Message Flag bit */
      DMRDataStruct->FullMessageFlag = (int)InputDataBit[71] & 1;

      /* -------- Available only on raw short data packet header -------- */
      /* Store the Bit Padding */
      DMRDataStruct->BitPadding = (int)ConvertBitIntoBytes(&InputDataBit[72], 8);

      fprintf(stderr, "| [Status/Precoded (S/P)] or [Raw] Short Data Header  [%s  Dst=%u  Src=%u  AB=%u  A=%u  SP=%u  DP=%u  [RAW:SARQ=%u  F=%u] OR [S/P:0x%03X]  SAP=%u (%s)] ",
                    DMRDataStruct->GroupDataCall ? "GRP":"IND",
                    DMRDataStruct->DestinationLogicalLinkID,
                    DMRDataStruct->SourceLogicalLinkID,
                    DMRDataStruct->AppendedBlocks,
                    DMRDataStruct->ResponseRequested,
                    DMRDataStruct->SourcePort,
                    DMRDataStruct->DestinationPort,
                    DMRDataStruct->SelecriveAutomaticRepeatReQuest,
                    DMRDataStruct->FullMessageFlag,
                    DMRDataStruct->StatusPrecoded,
                    DMRDataStruct->SAPIdentifier,
                    DmrDataServiceAccessPointIdentifierToStr(DMRDataStruct->SAPIdentifier));
      break;
    }

    /* Proprietary Data Packet */
    case 0b1111:
    {
      /* Store the SAP Identifier */
      DMRDataStruct->SAPIdentifier = (int)ConvertBitIntoBytes(&InputDataBit[0], 4);

      /* Store the Data Packet Format */
      DMRDataStruct->DataPacketFormat = (int)ConvertBitIntoBytes(&InputDataBit[4], 4);

      /* Store the Manufacturer Set ID (MFID) or FID (Feature Set ID) */
      DMRDataStruct->MFID = (int)ConvertBitIntoBytes(&InputDataBit[8], 8);

      /* Convert all bit received to bytes */
      for(i = 0; i < 12; i++) DataHeaderBytes[i] = (char)ConvertBitIntoBytes(&InputDataBit[i * 8], 8);

      /* Store the proprietary data (8 bytes) */
      for(i = 0; i < 8; i++) DMRDataStruct->ProprietaryData[i] = DataHeaderBytes[i + 2];

      fprintf(stderr, "| Proprietary Data Header  [SAP=%u  MFID=0x%02X  Data=%02X",
              DMRDataStruct->SAPIdentifier, DMRDataStruct->MFID, DataHeaderBytes[2]);

      for(i = 3; i < 10; i++)
      {
        fprintf(stderr, "-0x%02X", DataHeaderBytes[i] & 0xFF);
      }
      fprintf(stderr, "  ");

      fprintf(stderr, "CRC=0x%02X%02X] ", DataHeaderBytes[10], DataHeaderBytes[11]);
      break;
    }

    default:
    {
      /* Convert all bit received to bytes */
      for(i = 0; i < 12; i++) DataHeaderBytes[i] = (char)ConvertBitIntoBytes(&InputDataBit[i * 8], 8);

      fprintf(stderr, "| Data=0x%02X", DataHeaderBytes[0] & 0xFF);
      for(i = 1; i < 10; i++)
      {
        fprintf(stderr, "-0x%02X", DataHeaderBytes[i] & 0xFF);
      }
      fprintf(stderr, "  ");

      fprintf(stderr, "CRC=0x%02X%02X ", DataHeaderBytes[10], DataHeaderBytes[11]);
      break;
    }
  } /* End switch(DMRDataStruct->SAPIdentifier & 0b1111) */

} /* End DmrDataHeaderDecode() */


/*
 * @brief : This function convert the Service Access Point Identifier (SAP ID) to a
 *          explicit ASCII string.
 *
 * @param ServiceAccessPointIdentifier : The SAP ID (5 bit)
 *
 * @return A pointer with the SAP ID description (ASCII string format)
 *
 */
char * DmrDataServiceAccessPointIdentifierToStr(int ServiceAccessPointIdentifier)
{
  switch(ServiceAccessPointIdentifier & 0b1111)
  {
    case 0b0000: {return "Unified Data Transport (UDT)"; break;}
    case 0b0010: {return "TCP/IP header compression"; break;}
    case 0b0011: {return "UDP/IP header compression"; break;}
    case 0b0100: {return "IP based Packet data"; break;}
    case 0b0101: {return "Address Resolution Protocol (ARP)"; break;}
    case 0b1001: {return "Proprietary Packet data"; break;}
    case 0b1010: {return "Short Data"; break;}
    default:     {return "Unknown"; break;}
  } /* End switch(SAPIdentifier) */
} /* End DmrDataServiceAccessPointIdentifierToStr() */


/*
 * @brief : This function backup all necessary data before clearing
 *          all of them when receiving a new data header.
 *
 * @param opts : DSD options structure pointer
 *
 * @param state : DSD state structure pointer
 *
 * @return None
 *
 */
void DmrBackupReceivedData(dsd_opts * opts, dsd_state * state)
{
  UNUSED_VARIABLE(opts);
  UNUSED_VARIABLE(state);
} /* End DmrBackupReceivedData() */


/*
 * @brief : This function restore all necessary data after clearing
 *          all of them when receiving a new data header.
 *
 * @param opts : DSD options structure pointer
 *
 * @param state : DSD state structure pointer
 *
 * @return None
 *
 */
void DmrRestoreReceivedData(dsd_opts * opts, dsd_state * state)
{
  UNUSED_VARIABLE(opts);
  UNUSED_VARIABLE(state);
} /* End DmrRestoreReceivedData() */


/* End of file */
