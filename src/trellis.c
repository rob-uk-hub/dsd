/*
 *  Copyright (C) 2016 by Jonathan Naylor, G4KLX
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */
#include "dsd.h"
#include <stdbool.h>

const unsigned int INTERLEAVE_TABLE[] = {
    0U, 1U, 8U,   9U, 16U, 17U, 24U, 25U, 32U, 33U, 40U, 41U, 48U, 49U, 56U, 57U, 64U, 65U, 72U, 73U, 80U, 81U, 88U, 89U, 96U, 97U,
    2U, 3U, 10U, 11U, 18U, 19U, 26U, 27U, 34U, 35U, 42U, 43U, 50U, 51U, 58U, 59U, 66U, 67U, 74U, 75U, 82U, 83U, 90U, 91U,
    4U, 5U, 12U, 13U, 20U, 21U, 28U, 29U, 36U, 37U, 44U, 45U, 52U, 53U, 60U, 61U, 68U, 69U, 76U, 77U, 84U, 85U, 92U, 93U,
    6U, 7U, 14U, 15U, 22U, 23U, 30U, 31U, 38U, 39U, 46U, 47U, 54U, 55U, 62U, 63U, 70U, 71U, 78U, 79U, 86U, 87U, 94U, 95U};

const unsigned char ENCODE_TABLE[] = {
    0U,  8U, 4U, 12U, 2U, 10U, 6U, 14U,
    4U, 12U, 2U, 10U, 6U, 14U, 0U,  8U,
    1U,  9U, 5U, 13U, 3U, 11U, 7U, 15U,
    5U, 13U, 3U, 11U, 7U, 15U, 1U,  9U,
    3U, 11U, 7U, 15U, 1U,  9U, 5U, 13U,
    7U, 15U, 1U,  9U, 5U, 13U, 3U, 11U,
    2U, 10U, 6U, 14U, 0U,  8U, 4U, 12U,
    6U, 14U, 0U,  8U, 4U, 12U, 2U, 10U};

const unsigned char DECODE_TABLE[] = { //flip the rows and columns, right?
    0,4,1,5,3,7,2,6,
    8,12,9,13,11,15,10,14,
    4,2,5,3,7,1,6,0,
    12,10,13,11,15,9,14,8,
    2,6,3,7,1,5,0,4,
    10,14,11,15,9,13,8,12,
    6,0,7,1,5,3,4,2,
    14,8,15,9,13,11,12,10
};

const unsigned char BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])
#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])



bool CDMRTrellis_decode(const unsigned char* data, unsigned char* payload)
{
  assert(data != NULL);
  assert(payload != NULL);

  signed char dibits[98U];
  CDMRTrellis_deinterleave(data, dibits);

  unsigned char points[49U];
  CDMRTrellis_dibitsToPoints(dibits, points);

  // Check the original code
  unsigned char tribits[49U];
  unsigned int failPos = CDMRTrellis_checkCode(points, tribits);
  if (failPos == 999U)
  {
    CDMRTrellis_tribitsToBits(tribits, payload);
    return true;
  }

  unsigned char savePoints[49U];
  for (unsigned int i = 0U; i < 49U; i++)
    savePoints[i] = points[i];

  bool ret = CDMRTrellis_fixCode(points, failPos, payload);
  if (ret)
    return true;

  if (failPos == 0U)
    return false;

  // Backtrack one place for a last go
  return CDMRTrellis_fixCode(savePoints, failPos - 1U, payload);
}

void CDMRTrellis_encode(const unsigned char* payload, unsigned char* data)
{
    assert(payload != NULL);
    assert(data != NULL);

    unsigned char tribits[49U];
    CDMRTrellis_bitsToTribits(payload, tribits);

    unsigned char points[49U];
    unsigned char state = 0U;

    for (unsigned int i = 0U; i < 49U; i++) {
        unsigned char tribit = tribits[i];

        points[i] = ENCODE_TABLE[state * 8U + tribit];

        state = tribit;
    }

    signed char dibits[98U];
    CDMRTrellis_pointsToDibits(points, dibits);

    CDMRTrellis_interleave(dibits, data);
}

void CDMRTrellis_deinterleave(const unsigned char* data, signed char* dibits)
{
  unsigned int n;
  int i;

//  for (unsigned int i = 0U; i < 98U; i++)
//  {
//    n = i * 2U + 0U;
//    if (n >= 98U) n += 68U;
//    //bool b1 = READ_BIT(data, n) != 0x00U;
//    bool b1 = data[n] & 1;
//
//    n = i * 2U + 1U;
//    if (n >= 98U) n += 68U;
//    //bool b2 = READ_BIT(data, n) != 0x00U;
//    bool b2 = data[n] & 1;
//
//    signed char dibit;
//    if (!b1 && b2)
//      dibit = +3;
//    else if (!b1 && !b2)
//      dibit = +1;
//    else if (b1 && !b2)
//      dibit = -1;
//    else
//      dibit = -3;
//
//    n = INTERLEAVE_TABLE[i];
//    dibits[n] = dibit;
//  }

  //Keep it simple!
  for (i = 0; i < 98; i++)
  {
    //n = INTERLEAVE_TABLE[97-i];
    n = INTERLEAVE_TABLE[i];

    if (data[i] == 0)
    {
      dibits[n] =  1; //1
    }
    else if (data[i] == 1)
    {
      dibits[n] =  3; //3
    }
    else if (data[i] == 2)
    {
      dibits[n] =  -1; //-1
    }
    else /* (data[i] == 3) */
    {
      dibits[n] = -3; //-3
    }
  }
}

void CDMRTrellis_interleave(const signed char* dibits, unsigned char* data)
{
  for (unsigned int i = 0U; i < 98U; i++)
  {
    unsigned int n = INTERLEAVE_TABLE[i];

    bool b1, b2;
    switch (dibits[n])
    {
      case +3:
        b1 = false;
        b2 = true;
        break;
      case +1:
        b1 = false;
        b2 = false;
        break;
      case -1:
        b1 = true;
        b2 = false;
        break;
      default:
        b1 = true;
        b2 = true;
        break;
    }

    //n = i * 2U + 0U;
    //if (n >= 98U) n += 68U;
    //WRITE_BIT(data, n, b1);
    data[i * 2U + 0U] = (unsigned char)b1;

    //n = i * 2U + 1U;
    //if (n >= 98U) n += 68U;
    //WRITE_BIT(data, n, b2);
    data[i * 2U + 1U] = (unsigned char)b2;
  }
}

void CDMRTrellis_dibitsToPoints(const signed char* dibits, unsigned char* points)
{
  for (unsigned int i = 0U; i < 49U; i++)
  {
    if (dibits[i * 2U + 0U] == +1 && dibits[i * 2U + 1U] == -1)
      points[i] = 0U;
    else if (dibits[i * 2U + 0U] == -1 && dibits[i * 2U + 1U] == -1)
      points[i] = 1U;
    else if (dibits[i * 2U + 0U] == +3 && dibits[i * 2U + 1U] == -3)
      points[i] = 2U;
    else if (dibits[i * 2U + 0U] == -3 && dibits[i * 2U + 1U] == -3)
      points[i] = 3U;
    else if (dibits[i * 2U + 0U] == -3 && dibits[i * 2U + 1U] == -1)
      points[i] = 4U;
    else if (dibits[i * 2U + 0U] == +3 && dibits[i * 2U + 1U] == -1)
      points[i] = 5U;
    else if (dibits[i * 2U + 0U] == -1 && dibits[i * 2U + 1U] == -3)
      points[i] = 6U;
    else if (dibits[i * 2U + 0U] == +1 && dibits[i * 2U + 1U] == -3)
      points[i] = 7U;
    else if (dibits[i * 2U + 0U] == -3 && dibits[i * 2U + 1U] == +3)
      points[i] = 8U;
    else if (dibits[i * 2U + 0U] == +3 && dibits[i * 2U + 1U] == +3)
      points[i] = 9U;
    else if (dibits[i * 2U + 0U] == -1 && dibits[i * 2U + 1U] == +1)
      points[i] = 10U;
    else if (dibits[i * 2U + 0U] == +1 && dibits[i * 2U + 1U] == +1)
      points[i] = 11U;
    else if (dibits[i * 2U + 0U] == +1 && dibits[i * 2U + 1U] == +3)
      points[i] = 12U;
    else if (dibits[i * 2U + 0U] == -1 && dibits[i * 2U + 1U] == +3)
      points[i] = 13U;
    else if (dibits[i * 2U + 0U] == +3 && dibits[i * 2U + 1U] == +1)
      points[i] = 14U;
    else if (dibits[i * 2U + 0U] == -3 && dibits[i * 2U + 1U] == +1)
      points[i] = 15U;
  }
}

void CDMRTrellis_pointsToDibits(const unsigned char* points, signed char* dibits)
{
  for (unsigned int i = 0U; i < 49U; i++)
  {
    switch (points[i])
    {
      case 0U:
        dibits[i * 2U + 0U] = +1;
        dibits[i * 2U + 1U] = -1;
        break;
      case 1U:
        dibits[i * 2U + 0U] = -1;
        dibits[i * 2U + 1U] = -1;
        break;
      case 2U:
        dibits[i * 2U + 0U] = +3;
        dibits[i * 2U + 1U] = -3;
        break;
      case 3U:
        dibits[i * 2U + 0U] = -3;
        dibits[i * 2U + 1U] = -3;
        break;
      case 4U:
        dibits[i * 2U + 0U] = -3;
        dibits[i * 2U + 1U] = -1;
        break;
      case 5U:
        dibits[i * 2U + 0U] = +3;
        dibits[i * 2U + 1U] = -1;
        break;
      case 6U:
        dibits[i * 2U + 0U] = -1;
        dibits[i * 2U + 1U] = -3;
        break;
      case 7U:
        dibits[i * 2U + 0U] = +1;
        dibits[i * 2U + 1U] = -3;
        break;
      case 8U:
        dibits[i * 2U + 0U] = -3;
        dibits[i * 2U + 1U] = +3;
        break;
      case 9U:
        dibits[i * 2U + 0U] = +3;
        dibits[i * 2U + 1U] = +3;
        break;
      case 10U:
        dibits[i * 2U + 0U] = -1;
        dibits[i * 2U + 1U] = +1;
        break;
      case 11U:
        dibits[i * 2U + 0U] = +1;
        dibits[i * 2U + 1U] = +1;
        break;
      case 12U:
        dibits[i * 2U + 0U] = +1;
        dibits[i * 2U + 1U] = +3;
        break;
      case 13U:
        dibits[i * 2U + 0U] = -1;
        dibits[i * 2U + 1U] = +3;
        break;
      case 14U:
        dibits[i * 2U + 0U] = +3;
        dibits[i * 2U + 1U] = +1;
        break;
      default:
        dibits[i * 2U + 0U] = -3;
        dibits[i * 2U + 1U] = +1;
        break;
    }
  }
}

void CDMRTrellis_bitsToTribits(const unsigned char* payload, unsigned char* tribits)
{
  for (unsigned int i = 0U; i < 48U; i++)
  {
    //unsigned int n = 143U - i * 3U;

    //bool b1 = READ_BIT(payload, n) != 0x00U;
    //n--;
    //bool b2 = READ_BIT(payload, n) != 0x00U;
    //n--;
    //bool b3 = READ_BIT(payload, n) != 0x00U;

    bool b1 = (bool)payload[(3 * i) + 0U] & 1;
    bool b2 = (bool)payload[(3 * i) + 1U] & 1;
    bool b3 = (bool)payload[(3 * i) + 2U] & 1;

    unsigned char tribit = 0U;
    tribit |= b1 ? 4U : 0U;
    tribit |= b2 ? 2U : 0U;
    tribit |= b3 ? 1U : 0U;

    tribits[i] = tribit;
  }

  tribits[48U] = 0U;
}

void CDMRTrellis_tribitsToBits(const unsigned char* tribits, unsigned char* payload)
{
  for (unsigned int i = 0U; i < 48U; i++)
  {
    payload[(i * 3) + 0] = (tribits[i] >> 2) & 1;
    payload[(i * 3) + 1] = (tribits[i] >> 1) & 1;
    payload[(i * 3) + 2] = tribits[i] & 1;
  }
}

bool CDMRTrellis_fixCode(unsigned char* points, unsigned int failPos, unsigned char* payload)
{
  for (unsigned j = 0U; j < 20U; j++)
  {
    unsigned int bestPos = 0U;
    unsigned int bestVal = 0U;

    for (unsigned int i = 0U; i < 16U; i++)
    {
      points[failPos] = i;

      unsigned char tribits[49U];
      unsigned int pos = CDMRTrellis_checkCode(points, tribits);
      if (pos == 999U) {
        CDMRTrellis_tribitsToBits(tribits, payload);
        return true;
      }

      if (pos > bestPos)
      {
        bestPos = pos;
        bestVal = i;
      }
    }

    points[failPos] = bestVal;
    failPos = bestPos;
  }

  return false;
}

unsigned int CDMRTrellis_checkCode(const unsigned char* points, unsigned char* tribits)
{
  unsigned char state = 0U;

  for (unsigned int i = 0U; i < 49U; i++)
  {
    tribits[i] = 9U;

    for (unsigned int j = 0U; j < 8U; j++)
    {
      if (points[i] == ENCODE_TABLE[state * 8U + j])
      {
        tribits[i] = j;
        break;
      }
    }

    if (tribits[i] == 9U)
      return i;

    state = tribits[i];
  }

  if (tribits[48U] != 0U)
    return 48U;

  return 999U;
}

