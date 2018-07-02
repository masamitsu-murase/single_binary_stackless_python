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

/********* Language model for: Portuguese *********/

/**
 * Generated by BuildLangModel.py
 * On: 2017-03-27 19:40:04.365254
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
  SYM,  0, 17, 10,  7,  1, 16, 14, 18,  4, 28, 34, 12,  9,  6,  2, /* 4X */
   13, 21,  5,  3,  8, 11, 15, 32, 24, 31, 26,SYM,SYM,SYM,SYM,SYM, /* 5X */
  SYM,  0, 17, 10,  7,  1, 16, 14, 18,  4, 28, 34, 12,  9,  6,  2, /* 6X */
   13, 21,  5,  3,  8, 11, 15, 32, 24, 31, 26,SYM,SYM,SYM,SYM,CTR, /* 7X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 8X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 9X */
  SYM,SYM,SYM,SYM,SYM,SYM, 51,SYM, 52,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* AX */
  SYM,SYM,SYM,SYM, 53, 54,SYM,SYM, 55,SYM,SYM,SYM, 56, 57, 50,SYM, /* BX */
   36, 25, 35, 20, 41, 42, 43, 22, 39, 19, 29, 44, 58, 23, 45, 49, /* CX */
   47, 59, 46, 27, 37, 30, 38,SYM, 60, 61, 33, 62, 40, 63, 64, 48, /* DX */
   36, 25, 35, 20, 41, 42, 43, 22, 39, 19, 29, 44, 65, 23, 45, 49, /* EX */
   47, 66, 46, 27, 37, 30, 38,SYM, 67, 68, 33, 69, 40, 70, 71, 50, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const unsigned char Iso_8859_1_CharToOrderMap[] =
{
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,RET,CTR,CTR,RET,CTR,CTR, /* 0X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 1X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* 2X */
  NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,SYM,SYM,SYM,SYM,SYM,SYM, /* 3X */
  SYM,  0, 17, 10,  7,  1, 16, 14, 18,  4, 28, 34, 12,  9,  6,  2, /* 4X */
   13, 21,  5,  3,  8, 11, 15, 32, 24, 31, 26,SYM,SYM,SYM,SYM,SYM, /* 5X */
  SYM,  0, 17, 10,  7,  1, 16, 14, 18,  4, 28, 34, 12,  9,  6,  2, /* 6X */
   13, 21,  5,  3,  8, 11, 15, 32, 24, 31, 26,SYM,SYM,SYM,SYM,CTR, /* 7X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 8X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 9X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* AX */
  SYM,SYM,SYM,SYM,SYM, 72,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* BX */
   36, 25, 35, 20, 41, 42, 43, 22, 39, 19, 29, 44, 73, 23, 45, 49, /* CX */
   47, 74, 46, 27, 37, 30, 38,SYM, 75, 76, 33, 77, 40, 78, 79, 48, /* DX */
   36, 25, 35, 20, 41, 42, 43, 22, 39, 19, 29, 44, 80, 23, 45, 49, /* EX */
   47, 81, 46, 27, 37, 30, 38,SYM, 82, 83, 33, 84, 40, 85, 86, 50, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const unsigned char Windows_1252_CharToOrderMap[] =
{
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,RET,CTR,CTR,RET,CTR,CTR, /* 0X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 1X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* 2X */
  NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,SYM,SYM,SYM,SYM,SYM,SYM, /* 3X */
  SYM,  0, 17, 10,  7,  1, 16, 14, 18,  4, 28, 34, 12,  9,  6,  2, /* 4X */
   13, 21,  5,  3,  8, 11, 15, 32, 24, 31, 26,SYM,SYM,SYM,SYM,SYM, /* 5X */
  SYM,  0, 17, 10,  7,  1, 16, 14, 18,  4, 28, 34, 12,  9,  6,  2, /* 6X */
   13, 21,  5,  3,  8, 11, 15, 32, 24, 31, 26,SYM,SYM,SYM,SYM,CTR, /* 7X */
  SYM,ILL,SYM, 87,SYM,SYM,SYM,SYM,SYM,SYM, 88,SYM, 89,ILL, 90,ILL, /* 8X */
  ILL,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, 91,SYM, 92,ILL, 93, 50, /* 9X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* AX */
  SYM,SYM,SYM,SYM,SYM, 94,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* BX */
   36, 25, 35, 20, 41, 42, 43, 22, 39, 19, 29, 44, 95, 23, 45, 49, /* CX */
   47, 96, 46, 27, 37, 30, 38,SYM, 97, 98, 33, 99, 40,100,101, 48, /* DX */
   36, 25, 35, 20, 41, 42, 43, 22, 39, 19, 29, 44,102, 23, 45, 49, /* EX */
   47,103, 46, 27, 37, 30, 38,SYM,104,105, 33,106, 40,107,108, 50, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const unsigned char Iso_8859_9_CharToOrderMap[] =
{
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,RET,CTR,CTR,RET,CTR,CTR, /* 0X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 1X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* 2X */
  NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,SYM,SYM,SYM,SYM,SYM,SYM, /* 3X */
  SYM,  0, 17, 10,  7,  1, 16, 14, 18,  4, 28, 34, 12,  9,  6,  2, /* 4X */
   13, 21,  5,  3,  8, 11, 15, 32, 24, 31, 26,SYM,SYM,SYM,SYM,SYM, /* 5X */
  SYM,  0, 17, 10,  7,  1, 16, 14, 18,  4, 28, 34, 12,  9,  6,  2, /* 6X */
   13, 21,  5,  3,  8, 11, 15, 32, 24, 31, 26,SYM,SYM,SYM,SYM,CTR, /* 7X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 8X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 9X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* AX */
  SYM,SYM,SYM,SYM,SYM,109,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* BX */
   36, 25, 35, 20, 41, 42, 43, 22, 39, 19, 29, 44,110, 23, 45, 49, /* CX */
  111,112, 46, 27, 37, 30, 38,SYM,113,114, 33,115, 40,116,117, 48, /* DX */
   36, 25, 35, 20, 41, 42, 43, 22, 39, 19, 29, 44,118, 23, 45, 49, /* EX */
  119,120, 46, 27, 37, 30, 38,SYM,121,122, 33,123, 40,124,125, 50, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */


/* Model Table:
 * Total sequences: 893
 * First 512 sequences: 0.9952735790663951
 * Next 512 sequences (512-1024): 0.004726420933604831
 * Rest: 3.8163916471489756e-17
 * Negative sequences: TODO
 */
static const PRUint8 PortugueseLangModel[] =
{
  2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,0,3,3,3,3,0,3,2,3,0,0,3,2,2,3,0,0,0,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,0,2,3,3,2,3,2,3,2,3,0,2,3,3,2,2,2,0,0,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,2,2,3,2,3,2,3,2,3,0,2,3,3,0,3,0,0,0,
  3,3,3,3,3,2,3,3,3,3,3,3,3,3,2,2,3,3,3,3,3,3,0,3,0,3,2,3,0,2,3,3,2,2,3,0,0,0,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,2,3,3,3,0,3,3,3,3,2,3,3,2,2,2,3,2,0,0,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,2,3,2,3,3,3,2,2,3,3,0,2,
  3,3,3,3,3,2,3,3,3,2,3,3,2,2,3,3,3,2,3,3,3,3,3,3,2,3,3,3,3,2,0,3,2,3,3,2,0,3,
  3,3,3,3,3,3,2,3,2,3,2,3,2,2,3,2,2,2,2,3,3,2,0,3,0,3,0,3,2,3,2,3,3,3,0,2,0,2,
  3,3,3,3,3,3,3,0,3,3,3,3,3,2,2,2,2,2,3,3,3,0,0,3,0,3,2,3,0,3,2,3,3,2,2,3,0,3,
  3,3,3,3,3,2,3,2,2,3,2,3,2,3,2,0,2,3,0,3,3,2,0,3,0,3,2,3,0,2,2,3,2,3,0,3,0,3,
  3,3,3,2,3,3,3,2,3,3,3,3,3,2,2,0,2,2,3,3,2,2,3,3,0,3,2,3,0,3,2,3,0,2,3,3,0,2,
  3,3,3,3,3,3,3,3,3,3,3,2,3,3,3,3,3,3,2,3,2,2,3,3,3,2,3,0,3,3,0,2,2,0,2,0,0,0,
  3,3,3,3,3,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,2,3,0,3,2,3,0,3,0,3,2,2,2,3,0,3,
  3,3,3,3,3,3,2,2,3,0,2,3,3,3,0,0,0,2,3,3,2,2,3,3,0,3,2,3,0,2,2,2,0,3,0,2,0,2,
  3,3,3,3,3,3,3,2,2,3,2,3,3,2,2,2,0,2,3,3,2,0,0,2,0,3,0,2,0,3,2,3,2,2,0,2,0,0,
  3,3,3,0,3,3,0,2,0,0,0,3,0,0,0,2,0,0,0,3,2,0,0,3,0,3,0,2,0,3,2,0,0,0,0,2,0,2,
  3,3,3,2,3,3,0,2,2,2,2,3,3,2,2,0,3,2,0,3,0,0,0,3,0,2,0,3,0,3,0,2,0,2,0,0,0,2,
  3,3,3,3,3,3,3,3,3,2,2,3,3,2,2,2,3,2,2,3,2,0,0,2,0,2,2,2,3,2,0,2,2,2,0,0,0,0,
  3,3,3,3,3,3,3,2,3,2,0,3,3,0,0,0,2,2,2,2,3,0,0,2,0,3,0,2,0,0,3,3,2,0,2,0,0,0,
  2,2,2,3,2,3,3,3,3,3,3,2,3,3,2,2,2,0,0,0,0,0,0,0,2,0,2,0,0,0,0,0,0,0,0,0,0,0,
  0,2,3,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  2,0,0,2,0,0,0,0,0,0,0,3,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  3,0,3,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,3,0,0,2,0,0,0,0,
  3,0,3,3,0,3,3,3,3,3,3,0,3,3,3,3,3,3,0,0,0,2,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,
  3,3,3,2,3,0,0,0,3,0,3,3,2,3,0,3,2,0,2,2,2,0,0,2,3,2,0,2,2,0,2,0,0,0,0,0,0,2,
  0,0,0,3,0,3,2,2,3,0,3,2,3,3,3,3,3,3,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,
  3,3,3,0,3,2,2,0,0,2,2,3,0,0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,0,2,2,0,2,0,0,2,0,0,
  0,0,0,3,2,3,3,3,3,3,3,0,3,3,3,2,3,2,0,0,0,2,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,
  3,3,3,2,2,0,2,0,0,0,2,3,0,2,2,0,0,0,0,0,2,0,0,0,0,3,0,0,0,0,0,0,0,2,2,0,0,0,
  0,0,0,3,0,0,3,0,2,3,0,2,2,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  3,2,3,3,2,3,3,2,3,2,3,2,3,2,2,0,2,2,2,0,0,0,0,0,3,0,2,0,2,0,0,0,2,0,2,0,0,0,
  3,3,3,2,3,2,2,2,3,2,2,2,2,0,0,2,0,2,3,0,0,0,0,0,0,0,2,0,0,0,0,2,2,0,2,0,0,0,
  0,0,0,3,0,2,3,3,2,3,3,0,3,2,0,2,0,3,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,
  3,3,3,2,3,2,2,0,0,3,2,2,2,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,2,2,0,2,0,0,0,
  0,0,0,0,0,0,3,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,2,0,2,3,2,2,3,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};


const SequenceModel Iso_8859_15PortugueseModel =
{
  Iso_8859_15_CharToOrderMap,
  PortugueseLangModel,
  38,
  (float)0.9952735790663951,
  PR_TRUE,
  "ISO-8859-15"
};

const SequenceModel Iso_8859_1PortugueseModel =
{
  Iso_8859_1_CharToOrderMap,
  PortugueseLangModel,
  38,
  (float)0.9952735790663951,
  PR_TRUE,
  "ISO-8859-1"
};

const SequenceModel Windows_1252PortugueseModel =
{
  Windows_1252_CharToOrderMap,
  PortugueseLangModel,
  38,
  (float)0.9952735790663951,
  PR_TRUE,
  "WINDOWS-1252"
};

const SequenceModel Iso_8859_9PortugueseModel =
{
  Iso_8859_9_CharToOrderMap,
  PortugueseLangModel,
  38,
  (float)0.9952735790663951,
  PR_TRUE,
  "ISO-8859-9"
};