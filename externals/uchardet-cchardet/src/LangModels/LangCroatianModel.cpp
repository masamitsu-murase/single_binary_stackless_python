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

/********* Language model for: Croatian *********/

/**
 * Generated by BuildLangModel.py
 * On: 2017-03-27 18:49:48.502852
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
static const unsigned char Iso_8859_2_CharToOrderMap[] =
{
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,RET,CTR,CTR,RET,CTR,CTR, /* 0X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 1X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* 2X */
  NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,SYM,SYM,SYM,SYM,SYM,SYM, /* 3X */
  SYM,  0, 19, 20, 15,  2, 22, 17, 21,  1,  7,  9, 10, 12,  4,  3, /* 4X */
   14, 30,  6,  8,  5, 11, 13, 28, 29, 27, 16,SYM,SYM,SYM,SYM,SYM, /* 5X */
  SYM,  0, 19, 20, 15,  2, 22, 17, 21,  1,  7,  9, 10, 12,  4,  3, /* 6X */
   14, 30,  6,  8,  5, 11, 13, 28, 29, 27, 16,SYM,SYM,SYM,SYM,CTR, /* 7X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 8X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 9X */
  SYM, 49,SYM, 41,SYM, 50, 51,SYM,SYM, 23, 52, 53, 54,SYM, 24, 55, /* AX */
  SYM, 56,SYM, 41,SYM, 57, 58,SYM,SYM, 23, 59, 60, 61,SYM, 24, 62, /* BX */
   63, 39, 46, 64, 36, 65, 25, 40, 18, 31, 66, 45, 67, 68, 69, 70, /* CX */
   26, 71, 72, 47, 73, 74, 32,SYM, 75, 76, 48, 77, 33, 78, 79, 80, /* DX */
   81, 39, 46, 82, 36, 83, 25, 40, 18, 31, 84, 45, 85, 86, 87, 88, /* EX */
   26, 89, 90, 47, 91, 92, 32,SYM, 93, 94, 48, 95, 33, 96, 97,SYM, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const unsigned char Iso_8859_13_CharToOrderMap[] =
{
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,RET,CTR,CTR,RET,CTR,CTR, /* 0X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 1X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* 2X */
  NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,SYM,SYM,SYM,SYM,SYM,SYM, /* 3X */
  SYM,  0, 19, 20, 15,  2, 22, 17, 21,  1,  7,  9, 10, 12,  4,  3, /* 4X */
   14, 30,  6,  8,  5, 11, 13, 28, 29, 27, 16,SYM,SYM,SYM,SYM,SYM, /* 5X */
  SYM,  0, 19, 20, 15,  2, 22, 17, 21,  1,  7,  9, 10, 12,  4,  3, /* 6X */
   14, 30,  6,  8,  5, 11, 13, 28, 29, 27, 16,SYM,SYM,SYM,SYM,CTR, /* 7X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 8X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 9X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, 34,SYM, 98,SYM,SYM,SYM,SYM, 99, /* AX */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, 34,SYM,100,SYM,SYM,SYM,SYM,101, /* BX */
  102,103,104, 25, 36,105,106,107, 18, 31,108,109,110,111,112,113, /* CX */
   23,114,115, 47, 38, 37, 32,SYM,116, 41,117,118, 33,119, 24,120, /* DX */
  121,122,123, 25, 36,124,125,126, 18, 31,127,128,129,130,131,132, /* EX */
   23,133,134, 47, 38, 37, 32,SYM,135, 41,136,137, 33,138, 24,SYM, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const unsigned char Iso_8859_16_CharToOrderMap[] =
{
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,RET,CTR,CTR,RET,CTR,CTR, /* 0X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 1X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* 2X */
  NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,SYM,SYM,SYM,SYM,SYM,SYM, /* 3X */
  SYM,  0, 19, 20, 15,  2, 22, 17, 21,  1,  7,  9, 10, 12,  4,  3, /* 4X */
   14, 30,  6,  8,  5, 11, 13, 28, 29, 27, 16,SYM,SYM,SYM,SYM,SYM, /* 5X */
  SYM,  0, 19, 20, 15,  2, 22, 17, 21,  1,  7,  9, 10, 12,  4,  3, /* 6X */
   14, 30,  6,  8,  5, 11, 13, 28, 29, 27, 16,SYM,SYM,SYM,SYM,CTR, /* 7X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 8X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 9X */
  SYM,139,140, 41,SYM,SYM, 23,SYM, 23,SYM,141,SYM,142,SYM,143,144, /* AX */
  SYM,SYM, 18, 41, 24,SYM,SYM,SYM, 24, 18,145,SYM, 44, 44,146,147, /* BX */
   43, 39, 46,148, 36, 25,149, 40, 35, 31, 42, 45,150,151,152,153, /* CX */
   26,154,155, 47,156,157, 32,158,159,160, 48,161, 33,162,163,164, /* DX */
   43, 39, 46,165, 36, 25,166, 40, 35, 31, 42, 45,167,168,169,170, /* EX */
   26,171,172, 47,173,174, 32,175,176,177, 48,178, 33,179,180,181, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const unsigned char Windows_1250_CharToOrderMap[] =
{
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,RET,CTR,CTR,RET,CTR,CTR, /* 0X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 1X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* 2X */
  NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,SYM,SYM,SYM,SYM,SYM,SYM, /* 3X */
  SYM,  0, 19, 20, 15,  2, 22, 17, 21,  1,  7,  9, 10, 12,  4,  3, /* 4X */
   14, 30,  6,  8,  5, 11, 13, 28, 29, 27, 16,SYM,SYM,SYM,SYM,SYM, /* 5X */
  SYM,  0, 19, 20, 15,  2, 22, 17, 21,  1,  7,  9, 10, 12,  4,  3, /* 6X */
   14, 30,  6,  8,  5, 11, 13, 28, 29, 27, 16,SYM,SYM,SYM,SYM,CTR, /* 7X */
  SYM,ILL,SYM,ILL,SYM,SYM,SYM,SYM,ILL,SYM, 23,SYM,182,183, 24,184, /* 8X */
  ILL,SYM,SYM,SYM,SYM,SYM,SYM,SYM,ILL,SYM, 23,SYM,185,186, 24,187, /* 9X */
  SYM,SYM,SYM, 41,SYM,188,SYM,SYM,SYM,SYM,189,SYM,SYM,SYM,SYM,190, /* AX */
  SYM,SYM,SYM, 41,SYM,SYM,SYM,SYM,SYM,191,192,SYM,193,SYM,194,195, /* BX */
  196, 39, 46,197, 36,198, 25, 40, 18, 31,199, 45,200,201,202,203, /* CX */
   26,204,205, 47,206,207, 32,SYM,208,209, 48,210, 33,211,212,213, /* DX */
  214, 39, 46,215, 36,216, 25, 40, 18, 31,217, 45,218,219,220,221, /* EX */
   26,222,223, 47,224,225, 32,SYM,226,227, 48,228, 33,229,230,SYM, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const unsigned char Ibm852_CharToOrderMap[] =
{
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,RET,CTR,CTR,RET,CTR,CTR, /* 0X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 1X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* 2X */
  NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,SYM,SYM,SYM,SYM,SYM,SYM, /* 3X */
  SYM,  0, 19, 20, 15,  2, 22, 17, 21,  1,  7,  9, 10, 12,  4,  3, /* 4X */
   14, 30,  6,  8,  5, 11, 13, 28, 29, 27, 16,SYM,SYM,SYM,SYM,SYM, /* 5X */
  SYM,  0, 19, 20, 15,  2, 22, 17, 21,  1,  7,  9, 10, 12,  4,  3, /* 6X */
   14, 30,  6,  8,  5, 11, 13, 28, 29, 27, 16,SYM,SYM,SYM,SYM,CTR, /* 7X */
   40, 33, 31, 46, 36,231, 25, 40, 41, 45,232,233,234,235, 36, 25, /* 8X */
   31,236,237,238, 32,239,240,241,242, 32, 33,243,244, 41,SYM, 18, /* 9X */
   39,245, 47, 48,246,247, 24, 24,248,249,SYM,249, 18,249,SYM,SYM, /* AX */
  SYM,SYM,SYM,SYM,SYM, 39, 46,249,249,SYM,SYM,SYM,SYM,249,249,SYM, /* BX */
  SYM,SYM,SYM,SYM,SYM,SYM,249,249,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* CX */
   26, 26,249, 45,249,249,249,249,249,SYM,SYM,SYM,SYM,249,249,SYM, /* DX */
   47,249,249,249,249,249, 23, 23,249, 48,249,249,249,249,249,SYM, /* EX */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,249,249,249,SYM,SYM, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const unsigned char Maccentraleurope_CharToOrderMap[] =
{
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,RET,CTR,CTR,RET,CTR,CTR, /* 0X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 1X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* 2X */
  NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,SYM,SYM,SYM,SYM,SYM,SYM, /* 3X */
  SYM,  0, 19, 20, 15,  2, 22, 17, 21,  1,  7,  9, 10, 12,  4,  3, /* 4X */
   14, 30,  6,  8,  5, 11, 13, 28, 29, 27, 16,SYM,SYM,SYM,SYM,SYM, /* 5X */
  SYM,  0, 19, 20, 15,  2, 22, 17, 21,  1,  7,  9, 10, 12,  4,  3, /* 6X */
   14, 30,  6,  8,  5, 11, 13, 28, 29, 27, 16,SYM,SYM,SYM,SYM,CTR, /* 7X */
   36,249,249, 31,249, 32, 33, 39,249, 18, 36, 18, 25, 25, 31,249, /* 8X */
  249,249,249,249,249,249,249, 47,249,249, 32, 37, 48,249,249, 33, /* 9X */
  SYM,SYM,249,SYM,SYM,SYM,SYM,249,SYM,SYM,SYM,249,SYM,SYM,249,249, /* AX */
  249,249,SYM,SYM,249,249,SYM,SYM, 41,249,249,249,249,249,249,249, /* BX */
  249,249,SYM,SYM,249,249,SYM,SYM,SYM,SYM,SYM,249,249, 37,249, 38, /* CX */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, 38,249,249,249,SYM,SYM,249,249, /* DX */
  249, 23,SYM,SYM, 23,249,249, 39,249,249,249, 24, 24,249, 47,249, /* EX */
  249,249, 48,249,249,249,249,249,249,249,249,249, 41,249,249,SYM, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */


/* Model Table:
 * Total sequences: 713
 * First 512 sequences: 0.9989823195946479
 * Next 512 sequences (512-1024): 0.001017680405352139
 * Rest: -3.2959746043559335e-17
 * Negative sequences: TODO
 */
static const PRUint8 CroatianLangModel[] =
{
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,0,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,0,2,2,2,0,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,0,0,3,3,2,0,0,0,0,3,2,0,0,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,3,2,0,0,
  3,3,3,3,3,3,3,3,3,3,2,3,3,3,3,2,3,0,3,3,2,2,2,3,0,0,0,0,0,0,0,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,2,2,2,3,3,3,3,0,0,0,0,3,2,0,2,
  3,3,3,3,3,3,3,3,3,0,3,3,3,3,2,2,0,3,3,0,3,2,0,3,0,2,0,2,3,0,0,
  3,3,3,3,3,3,0,3,3,3,3,3,3,3,2,3,3,3,0,3,3,3,3,0,2,0,0,3,0,2,0,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,3,3,3,3,2,2,0,
  3,3,3,3,3,2,3,3,3,2,3,3,3,2,3,3,0,3,2,3,3,2,3,0,0,0,0,2,3,0,0,
  3,3,3,3,3,0,3,3,3,3,3,3,2,0,2,3,0,0,2,0,3,0,0,3,0,0,0,2,0,0,0,
  3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,0,2,2,2,2,3,3,0,2,0,3,0,2,0,0,0,
  3,3,3,3,3,3,3,3,3,0,3,3,3,3,0,3,3,3,0,3,2,2,3,0,3,0,0,2,3,2,2,
  3,3,3,3,3,0,3,3,2,0,3,3,3,3,0,3,2,3,0,3,0,3,0,0,0,0,0,2,2,0,0,
  3,3,3,3,3,2,3,2,2,0,3,3,3,3,2,3,3,3,0,0,0,3,2,0,0,0,0,3,2,0,0,
  3,3,3,3,3,0,2,3,0,3,3,3,0,3,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,
  3,3,3,3,3,2,3,3,3,0,3,3,2,2,0,3,3,0,0,2,3,0,3,0,0,0,0,2,0,0,2,
  3,3,3,3,2,3,3,3,3,3,3,3,3,3,0,3,0,2,0,0,0,3,0,0,0,0,0,2,0,0,3,
  3,3,3,3,3,3,3,0,3,2,3,3,2,3,0,2,3,2,0,3,3,2,2,0,0,0,0,3,3,2,0,
  3,3,3,3,3,3,3,0,3,2,3,3,2,0,2,2,0,2,0,0,0,0,3,0,0,0,0,0,0,0,0,
  3,3,3,2,3,3,2,0,0,3,3,3,2,3,3,0,0,0,2,0,2,0,0,0,0,3,0,0,0,0,0,
  3,3,3,3,3,0,0,2,0,0,2,3,0,0,0,3,0,0,0,3,0,0,0,0,0,0,2,0,0,0,0,
  3,3,3,3,3,0,0,0,0,2,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,
  3,3,3,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  3,2,3,3,3,2,2,2,3,0,3,3,0,0,0,2,2,2,0,0,2,0,0,0,0,0,0,0,0,0,0,
  3,3,3,3,2,3,2,2,2,0,0,3,0,0,0,0,0,0,0,2,2,3,2,0,0,0,0,2,2,0,0,
  3,3,2,0,0,0,2,0,0,0,0,2,0,2,3,0,0,2,0,0,0,0,2,0,0,0,0,0,3,0,0,
  0,3,2,0,0,0,2,0,0,0,0,3,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,
};


const SequenceModel Iso_8859_2CroatianModel =
{
  Iso_8859_2_CharToOrderMap,
  CroatianLangModel,
  31,
  (float)0.9989823195946479,
  PR_TRUE,
  "ISO-8859-2"
};

const SequenceModel Iso_8859_13CroatianModel =
{
  Iso_8859_13_CharToOrderMap,
  CroatianLangModel,
  31,
  (float)0.9989823195946479,
  PR_TRUE,
  "ISO-8859-13"
};

const SequenceModel Iso_8859_16CroatianModel =
{
  Iso_8859_16_CharToOrderMap,
  CroatianLangModel,
  31,
  (float)0.9989823195946479,
  PR_TRUE,
  "ISO-8859-16"
};

const SequenceModel Windows_1250CroatianModel =
{
  Windows_1250_CharToOrderMap,
  CroatianLangModel,
  31,
  (float)0.9989823195946479,
  PR_TRUE,
  "WINDOWS-1250"
};

const SequenceModel Ibm852CroatianModel =
{
  Ibm852_CharToOrderMap,
  CroatianLangModel,
  31,
  (float)0.9989823195946479,
  PR_TRUE,
  "IBM852"
};

const SequenceModel MaccentraleuropeCroatianModel =
{
  Maccentraleurope_CharToOrderMap,
  CroatianLangModel,
  31,
  (float)0.9989823195946479,
  PR_TRUE,
  "MacCentralEurope"
};