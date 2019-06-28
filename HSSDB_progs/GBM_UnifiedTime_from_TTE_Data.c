
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.


#include "HSSDB_Progs_Header.h"

//  Michael S. Briggs, 2008 Oct 2, UAH / NSSTC.

//  See ProcessTTE.c for comments about the format of TTE and esp.
//  the calculation of time from TTE data.

//  Compare to IntegerTime_from_CoarseFine.c


//   Both arguments are input, output is solely by function return.


long long unsigned int GBM_UnifiedTime_from_TTE_Data (

   uint32_t HeaderCoarseTime,
   uint32_t TTE_TimeWord,
   uint32_t TTE_DataWord

) {

   const long long unsigned int Ticks_per_Coarse = 50000;

   uint32_t CoarseTime;
   uint16_t FineTime;


   CoarseTime = ( HeaderCoarseTime & 0xF0000000 )  |  ( TTE_TimeWord & 0x0FFFFFFF );

   FineTime = (uint16_t)  ( TTE_DataWord >> 16 );

   //  Multiply GBM coarse time by the number of 2 microsecond
   //  "ticks" per coarse time unit of 0.1 s.

   return ( Ticks_per_Coarse * CoarseTime + FineTime );

}
