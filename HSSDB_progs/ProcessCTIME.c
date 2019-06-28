
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.


//  Michael S. Briggs, 2004 Oct 8, UAH / NSSTC.

#include <stdio.h>

#include "HSSDB_Progs_Header.h"



void ProcessCTIME (

   uint16_t PacketData [],
   uint16_t PacketDataLength,
   FILE * ptr_to_SummaryFile

) {

   uint32_t SumCounts [ NUM_DET ];
   uint32_t chan;
   uint32_t Det;


   // NOTE!! The data of CSPEC and CTIME DO NOT need byte-swapping --
   // these datatypes are natively little endian !!!

   if ( PacketDataLength  !=  2 * NUM_TIME_CHAN * NUM_DET ) {

      printf ( "\n **** ERROR: CTIME packet with incorrect amount of data: %u\n", PacketDataLength );
      fprintf ( ptr_to_SummaryFile, "\n **** ERROR: CTIME packet with incorrect amount of data: %u\n", PacketDataLength );

   } else {  //  valid PacketDataLength ?

      for ( Det=0; Det < NUM_DET; Det++ )  SumCounts [Det] = 0;

      for ( Det=0; Det < NUM_DET; Det++ ) {
         for ( chan = 0; chan < NUM_TIME_CHAN; chan++ )  SumCounts [Det] += PacketData [ Det * NUM_TIME_CHAN + chan ];
      }


      fprintf ( ptr_to_SummaryFile, " CTIME Counts for Detector 0:\n" );
      for ( chan=0; chan < NUM_TIME_CHAN; chan++ )
         fprintf ( ptr_to_SummaryFile, " %7u", PacketData [chan] );
      fprintf ( ptr_to_SummaryFile, "\n" );

      fprintf ( ptr_to_SummaryFile, "CTIME Counts from channel 0 to 7 for each Detector:\n Det 0 to  6:");
      for ( Det=0; Det <=  6; Det++ )  fprintf ( ptr_to_SummaryFile, " %8u", SumCounts [Det] );
      fprintf ( ptr_to_SummaryFile, "\n Det 7 to 13:");
      for ( Det=7; Det < NUM_DET; Det++ )  fprintf  (ptr_to_SummaryFile, " %8u", SumCounts [Det] );
      fprintf ( ptr_to_SummaryFile, "\n" );


   }  //  valid PacketDataLength ?


   return;

}  // ProcessCTIME ()