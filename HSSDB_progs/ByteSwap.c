
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.

//  Michael S. Briggs, 2004 Oct 8, UAH / NSSTC.

//  These two functions perform byte swapping, for converting endianess.
//  For example, they can convert big-endian data that was read as words
//  to the little-endian representation used by Intel processors.

//   One functions is for 2-byte words (e.g., short ints), the other
//   for 4-byte words (e.g., long ints, floats).


#include "HSSDB_Progs_Header.h"



//  The argument is solely input; output is solely by function return:

uint16_t  ByteSwap_2  ( uint16_t  Word_2 ) {


   uint16_t Byte1, Byte2, result;


   Byte1 =  Word_2       & (uint16_t) 0xFF;
   Byte2 = (Word_2 >> 8) & (uint16_t) 0xFF;

   result = (uint16_t) (Byte1 << 8)  |  Byte2;

   return ( result );

}



//  The argument is solely input; output is solely by function return:

uint32_t  ByteSwap_4  ( unsigned  int  Word_4 ) {


   uint32_t Byte1, Byte2, Byte3, Byte4;
   uint32_t Answer;


   Byte1 =  Word_4        & 0xFF;
   Byte2 = (Word_4 >>  8) & 0xFF;
   Byte3 = (Word_4 >> 16) & 0xFF;
   Byte4 = (Word_4 >> 24) & 0xFF;

   Answer  = Byte4;
   Answer |= Byte3 <<  8;
   Answer |= Byte2 << 16;
   Answer |= Byte1 << 24;

   return ( Answer );

}
