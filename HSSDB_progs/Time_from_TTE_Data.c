
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.


#include "HSSDB_Progs_Header.h"

//  Michael S. Briggs, 2004 Oct 8, UAH / NSSTC.

//  See ProcessTTE.c for comments about the format of TTE and esp.
//  the calculation of time from TTE data.

long double  Time_from_TTE_Data (

   uint32_t HeaderCoarseTime,
   uint32_t TTE_TimeWord,
   uint32_t TTE_DataWord

) {

   uint32_t CoarseTime;
   uint16_t FineTime;


   CoarseTime = ( HeaderCoarseTime & 0xF0000000 )  |  ( TTE_TimeWord & 0x0FFFFFFF );

   FineTime = (uint16_t) ( TTE_DataWord >> 16 );

   return ( FloatTime_from_CoarseFine ( CoarseTime, FineTime ) );

}
