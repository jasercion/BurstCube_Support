
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.


//  Michael S. Briggs, 2007 September 20--??,  UAH / NSSTC / GBM.


//  Initial notes:  all time calculations are in "ticks" of 2 microseconds,
//  the unit of GBM fine time.
//  "unsigned long long ints" are used, so that very large times, and time differences
//  can be represented in the units of ticks.   If "unsigned long ints" were unused, i.e.,
//  32 bits words, we could only go up to 2 hours 23 minutes 9.9 s.   With "unsigned long
//  long ints", which should be 64 bit words, we can represent times as large as necessary.




#include "HSSDB_Progs_Header.h"

#include <math.h>
#include <limits.h>


//   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***


void Bin_TTE (

   // Input arguments:
   uint16_t * Number_TTE_DataWords_ptr,
   uint32_t TTE_CoarseTime [],
   uint16_t TTE_FineTime [],
   uint16_t TTE_Channel [],
   uint16_t TTE_Detector []

) {


   //  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **


   static _Bool  FirstCall = true;

   static uint64_t  BaseTimeTicks = 0;

   const  uint64_t  Ticks_per_TenthSec =  50000;


   uint64_t TimeTicks;

   uint32_t TimeBin;

   static uint32_t Counts_by_Det [14] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };



   //   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***

   //  All times are measured from a Base Time defined as the time of the first TTE
   //  data word in the file:

   if ( FirstCall ) {

      FirstCall = false;

      BaseTimeTicks = Ticks_per_TenthSec * TTE_CoarseTime [0] + TTE_FineTime [0];

   }

   //  End first call initialization.




   //  Loop through the data words that were passed to the routine on this call,
   //  typically the data words of a TTE packet:

   for ( i_tte=0;  i_tte < *Number_TTE_DataWords_ptr;  i_tte++ ) {

      TimeTicks = Ticks_per_TenthSec * TTE_CoarseTime [i_tte] + TTE_FineTime [i_tte];

      TimeOffset_Ticks = TimeTicks - BaseTimeTicks;

      TimeBin = TimeOffset_Ticks / ??

      if ( TimeBin == Prev_TimeBin ) {
      ?????? also test if count is within an channel range ???
         Counts_by_Det [ TTE_Detector [i_tte] ] ++

      } else {

         ???? previous histogram is complete -- "super-accumulate" it.
         ???  Don't forget to accumulate the count.

      }


   }  // i_tte loop



}  //  Bin_TTE ()
