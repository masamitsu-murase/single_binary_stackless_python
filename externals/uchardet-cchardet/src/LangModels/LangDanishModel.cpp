/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "../nsSBCharSetProber.h"

/********* Language model for: Danish *********/

/**
 * Generated by BuildLangModel.py
 * On: 2017-03-27 17:44:30.578549
 **/

/* Character Mapping Table:
 * ILL: illegal character.
 * CTR: control character specific to the charset.
 * RET: carriage/return.
 * SYM: symbol (punctuation) that does not belong to word.
 * NUM: 0 - 9.
 *
 * Other characters are ordered by probabilities
 * (0 is the most common character in the language).
 *
 * Orders are generic to a language. So the codepoint with order X in
 * CHARSET1 maps to the same character as the codepoint with the same
 * order X in CHARSET2 for the same language.
 * As such, it is possible to get missing order. For instance the
 * ligature of 'o' and 'e' exists in ISO-8859-15 but not in ISO-8859-1
 * even though they are both used for French. Same for the euro sign.
 */
static const unsigned char Iso_8859_15_CharToOrderMap[] =
{
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,RET,CTR,CTR,RET,CTR,CTR, /* 0X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 1X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* 2X */
  NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,SYM,SYM,SYM,SYM,SYM,SYM, /* 3X */
  SYM,  5, 16, 23,  7,  0, 13, 10, 18,  4, 24, 12,  8, 11,  2,  9, /* 4X */
   17, 29,  1,  6,  3, 15, 14, 25, 27, 21, 26,SYM,SYM,SYM,SYM,SYM, /* 5X */
  SYM,  5, 16, 23,  7,  0, 13, 10, 18,  4, 24, 12,  8, 11,  2,  9, /* 6X */
   17, 29,  1,  6,  3, 15, 14, 25, 27, 21, 26,SYM,SYM,SYM,SYM,CTR, /* 7X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 8X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 9X */
  SYM,SYM,SYM,SYM,SYM,SYM, 35,SYM, 35,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* AX */
  SYM,SYM,SYM,SYM, 51, 52,SYM,SYM, 53,SYM,SYM,SYM, 54, 55, 56,SYM, /* BX */
   48, 32, 33, 49, 34, 22, 20, 37, 38, 28, 42, 46, 57, 39, 50, 43, /* CX */
   45, 47, 58, 36, 40, 59, 31,SYM, 19, 60, 41, 61, 30, 62, 63, 44, /* DX */
   48, 32, 33, 49, 34, 22, 20, 37, 38, 28, 42, 46, 64, 39, 50, 43, /* EX */
   45, 47, 65, 36, 40, 66, 31,SYM, 19, 67, 41, 68, 30, 69, 70, 71, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const unsigned char Iso_8859_1_CharToOrderMap[] =
{
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,RET,CTR,CTR,RET,CTR,CTR, /* 0X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 1X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* 2X */
  NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,SYM,SYM,SYM,SYM,SYM,SYM, /* 3X */
  SYM,  5, 16, 23,  7,  0, 13, 10, 18,  4, 24, 12,  8, 11,  2,  9, /* 4X */
   17, 29,  1,  6,  3, 15, 14, 25, 27, 21, 26,SYM,SYM,SYM,SYM,SYM, /* 5X */
  SYM,  5, 16, 23,  7,  0, 13, 10, 18,  4, 24, 12,  8, 11,  2,  9, /* 6X */
   17, 29,  1,  6,  3, 15, 14, 25, 27, 21, 26,SYM,SYM,SYM,SYM,CTR, /* 7X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 8X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 9X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* AX */
  SYM,SYM,SYM,SYM,SYM, 72,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* BX */
   48, 32, 33, 49, 34, 22, 20, 37, 38, 28, 42, 46, 73, 39, 50, 43, /* CX */
   45, 47, 74, 36, 40, 75, 31,SYM, 19, 76, 41, 77, 30, 78, 79, 44, /* DX */
   48, 32, 33, 49, 34, 22, 20, 37, 38, 28, 42, 46, 80, 39, 50, 43, /* EX */
   45, 47, 81, 36, 40, 82, 31,SYM, 19, 83, 41, 84, 30, 85, 86, 87, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const unsigned char Windows_1252_CharToOrderMap[] =
{
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,RET,CTR,CTR,RET,CTR,CTR, /* 0X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 1X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* 2X */
  NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,SYM,SYM,SYM,SYM,SYM,SYM, /* 3X */
  SYM,  5, 16, 23,  7,  0, 13, 10, 18,  4, 24, 12,  8, 11,  2,  9, /* 4X */
   17, 29,  1,  6,  3, 15, 14, 25, 27, 21, 26,SYM,SYM,SYM,SYM,SYM, /* 5X */
  SYM,  5, 16, 23,  7,  0, 13, 10, 18,  4, 24, 12,  8, 11,  2,  9, /* 6X */
   17, 29,  1,  6,  3, 15, 14, 25, 27, 21, 26,SYM,SYM,SYM,SYM,CTR, /* 7X */
  SYM,ILL,SYM, 88,SYM,SYM,SYM,SYM,SYM,SYM, 35,SYM, 89,ILL, 90,ILL, /* 8X */
  ILL,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, 35,SYM, 91,ILL, 92, 93, /* 9X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* AX */
  SYM,SYM,SYM,SYM,SYM, 94,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* BX */
   48, 32, 33, 49, 34, 22, 20, 37, 38, 28, 42, 46, 95, 39, 50, 43, /* CX */
   45, 47, 96, 36, 40, 97, 31,SYM, 19, 98, 41, 99, 30,100,101, 44, /* DX */
   48, 32, 33, 49, 34, 22, 20, 37, 38, 28, 42, 46,102, 39, 50, 43, /* EX */
   45, 47,103, 36, 40,104, 31,SYM, 19,105, 41,106, 30,107,108,109, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */


/* Model Table:
 * Total sequences: 938
 * First 512 sequences: 0.9968629865699213
 * Next 512 sequences (512-1024): 0.0031370134300787167
 * Rest: -5.551115123125783e-17
 * Negative sequences: TODO
 */
static const PRUint8 DanishLangModel[] =
{
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,2,3,2,3,3,3,2,3,0,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,2,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,2,0,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,2,0,3,3,2,3,2,2,2,
  3,3,3,3,3,3,3,3,3,2,3,3,3,3,3,3,3,3,3,2,0,3,0,3,3,3,3,3,0,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,2,0,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,3,2,0,2,2,
  3,3,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,2,3,0,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,0,3,0,3,3,3,2,2,0,2,
  3,3,3,3,3,3,3,3,3,3,3,3,2,3,3,3,3,2,3,3,3,3,3,2,3,2,0,0,2,2,
  3,3,3,3,3,3,3,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,0,2,2,2,2,0,
  3,3,3,3,3,3,3,2,3,3,2,3,3,2,3,3,2,2,3,3,3,3,3,2,2,2,0,2,2,0,
  3,3,3,3,3,3,3,3,3,3,3,2,2,3,3,3,2,2,3,3,3,3,3,2,3,0,0,2,2,0,
  3,3,3,3,3,3,3,3,3,3,3,2,2,2,2,3,2,2,2,3,3,2,3,2,3,0,0,0,2,0,
  3,3,3,3,3,3,3,3,3,2,3,3,3,3,3,2,3,3,3,2,2,2,0,3,2,2,2,3,0,2,
  3,3,3,3,3,3,3,3,3,3,2,2,2,2,0,3,3,0,2,3,3,3,3,2,3,2,2,0,2,0,
  3,3,3,3,3,3,3,3,3,3,3,3,2,3,2,3,3,3,3,3,3,3,3,3,2,2,2,2,2,0,
  3,3,3,3,3,3,2,2,3,3,2,3,2,2,3,3,2,2,2,3,3,3,3,2,3,2,0,0,2,0,
  3,3,3,3,2,2,3,3,3,2,3,3,3,2,3,2,3,2,2,2,0,2,0,2,3,0,2,0,0,0,
  2,3,3,3,3,2,3,3,3,3,3,3,3,3,3,2,3,2,2,0,0,0,0,2,0,0,0,0,0,0,
  3,3,3,3,2,3,3,3,3,3,3,3,3,2,3,2,3,3,3,2,0,0,0,2,2,2,3,0,0,0,
  3,3,3,3,0,0,3,3,3,2,3,2,3,2,3,0,3,2,2,0,0,0,0,0,0,0,0,0,0,0,
  3,3,2,3,3,3,3,2,3,3,2,2,3,2,2,3,2,2,3,0,2,3,0,3,2,2,2,0,0,2,
  3,3,3,3,2,3,3,3,3,3,2,2,2,2,2,3,2,2,2,3,3,3,0,0,2,0,2,0,0,0,
  3,2,2,2,3,3,2,2,2,3,0,2,2,2,0,2,2,2,2,0,0,2,0,2,0,2,2,0,0,0,
  3,2,2,2,3,3,2,2,2,3,2,2,2,2,2,2,2,2,3,0,0,2,0,2,2,2,3,0,2,0,
  3,2,0,2,3,3,2,0,2,2,0,0,0,2,2,2,0,2,2,2,0,2,0,2,0,2,0,2,0,0,
  2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,0,0,2,2,0,0,0,0,2,0,0,0,0,0,0,
  2,0,0,0,0,2,0,0,0,2,0,0,0,0,2,3,2,0,0,0,0,0,0,0,0,0,0,0,0,2,
};


const SequenceModel Iso_8859_15DanishModel =
{
  Iso_8859_15_CharToOrderMap,
  DanishLangModel,
  30,
  (float)0.9968629865699213,
  PR_TRUE,
  "ISO-8859-15"
};

const SequenceModel Iso_8859_1DanishModel =
{
  Iso_8859_1_CharToOrderMap,
  DanishLangModel,
  30,
  (float)0.9968629865699213,
  PR_TRUE,
  "ISO-8859-1"
};

const SequenceModel Windows_1252DanishModel =
{
  Windows_1252_CharToOrderMap,
  DanishLangModel,
  30,
  (float)0.9968629865699213,
  PR_TRUE,
  "WINDOWS-1252"
};