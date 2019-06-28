
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.


//  Michael S. Briggs, 2007 October 9,  UAH / NSSTC / GBM.
//  Comments updated 2008 June 22.


//  This program, in multiple calls, receives as input TTE events.
//  It outputs the TTE events to binary/unformatted files (i.e., C fwrite),
//  one file per detector.
//  There is an important reason for writing the data to separate files by detector.
//  Since the data is read in a round-robin fashion by the FPGA on the GBM DPU,
//  TTE events from different detectors may not be in time order.  This problem
//  only occurs when between events from different detectors -- so a first step
//  in fixing the problem is to separate the common TTE data stream into separate streams,
//  one for each detector.

//  A secondary action of this program is to output the first and last times of the
//  data stream.


#include <limits.h>


#include "HSSDB_Progs_Header.h"



//   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***


void Output_TTE (   // all arguments are input:

   uint16_t Number_TTE_DataWords,
   uint32_t TTE_CoarseTime [],
   uint16_t TTE_FineTime [],
   uint16_t TTE_Channel [],
   uint16_t TTE_Detector [],
   char * Analysis_FileName_ptr,
   FILE * ptr_to_AnalysisFile,
   FILE * ptr_to_SummaryFile

) {

   //  ----------------------------------------------------------------------------------

   //  These variables have to be "static" because this routine is called once per packet,
   //  but these variables need to retain information across packets, and thus across calls
   //  to this routine.

   static  uint32_t  TTE_count_by_det [NUM_DET];

   static  uint32_t  First_Time_Coarse;
   static  uint16_t First_Time_Fine;
   static  uint64_t  First_CombinedTime = UINT64_MAX;

   static  uint32_t  Last_Time_Coarse;
   static  uint16_t Last_Time_Fine;
   static  uint64_t  Last_CombinedTime = 0;

   static DetectorFile_type  DetectorFiles [NUM_DET];
   static _Bool FirstCall = true;


   //  ----------------------------------------------------------------------------------

   uint32_t i_tte;
   uint32_t j_det, this_det;

   uint64_t  ThisTime;

   TTE_Data_type  TTE_Data;

   size_t num_written;

   uint32_t Detector_Output_Cnt;

   uint64_t  Total_TTE_count;

   char WorkString [16];
   size_t  FileName_Length;

   long double  JulianDay;
   int32_t  year, month, day, hours, minutes;
   long double  seconds;

   //  ----------------------------------------------------------------------------------


   //  On first call,
   //  1) initialize structure array DetectorFiles --
   //       for each detector, no TTE events have been encountered yet,
   //       and create filename of output file,
   //  2) array TTE_count_by_det that records how many TTE events were output.

   if ( FirstCall ) {

      FirstCall = false;

      for ( j_det=0;  j_det < NUM_DET;  j_det++ ) {

         DetectorFiles [j_det] .DetectorFile_Opened = false;

         DetectorFiles [j_det] .DetectorFileName_ptr = malloc (120);
         if ( DetectorFiles [j_det] .DetectorFileName_ptr == NULL ) {
            printf ( "\n\nmalloc to string failed. exiting.\n" );
            exit (1);
         }

         sprintf ( WorkString, ".TTE_Det_%02u.dat", j_det );
         FileName_Length = strlen ( Analysis_FileName_ptr );
         memcpy ( DetectorFiles [j_det] .DetectorFileName_ptr, Analysis_FileName_ptr, FileName_Length );
         strncpy ( DetectorFiles [j_det] .DetectorFileName_ptr + FileName_Length - 4, WorkString, 15 );

         TTE_count_by_det [j_det] = 0;

      }  // j_det

   }  // FirstCall ?



   if ( Number_TTE_DataWords == UINT16_MAX ) {  // Closeout / Normal case ?

      //  ***** Closeout case:
      //  UINT16_MAX is a special value used by the calling routine to
      //  signal that there is no more data and that routine should
      //  perform its close out actions.    (The maxium number of TTE words
      //  per packet is approx. 1020, so it is impossible for a real
      //  value for this variable from the data to have the value UINT16_MAX.)
      //  The closeout action is to output some summary information.

      JulianDay = GBM_MET_Time_to_JulianDay ( First_Time_Coarse, First_Time_Fine );
      JulianDay_to_Calendar_subr ( JulianDay, &year, &month, &day, &hours, &minutes, &seconds );
      printf ( "\nEarliest time in the TTE data:\n %11u : %6u  = %Lf  = %4d:%02d:%02d at %02d:%02d:%9Lf\n",
         First_Time_Coarse, First_Time_Fine, JulianDay, year, month, day, hours, minutes, seconds );
      fprintf ( ptr_to_AnalysisFile,
          "\nEarliest time in the TTE data:\n %11u : %6u  =  %20llu  =  %Lf  = %4d:%02d:%02d at %02d:%02d:%9Lf\n",
          First_Time_Coarse, First_Time_Fine, IntegerTime_from_CoarseFine ( First_Time_Coarse, First_Time_Fine ),
          JulianDay, year, month, day, hours, minutes, seconds );
      fprintf ( ptr_to_SummaryFile,
          "\nEarliest time in the TTE data:\n %11u : %6u  =  %20llu  =  %Lf  = %4d:%02d:%02d at %02d:%02d:%9Lf\n",
          First_Time_Coarse, First_Time_Fine, IntegerTime_from_CoarseFine ( First_Time_Coarse, First_Time_Fine ),
          JulianDay, year, month, day, hours, minutes, seconds );


      JulianDay = GBM_MET_Time_to_JulianDay ( Last_Time_Coarse, Last_Time_Fine );
      JulianDay_to_Calendar_subr ( JulianDay, &year, &month, &day, &hours, &minutes, &seconds );
      printf ( "\nLast time in the TTE data:\n %11u : %6u  = %Lf  = %4d:%02d:%02d at %02d:%02d:%9Lf\n",
         Last_Time_Coarse, Last_Time_Fine, JulianDay, year, month, day, hours, minutes, seconds );
      fprintf ( ptr_to_AnalysisFile,
          "\nLast time in the TTE data:\n%11u : %6u  =  %20llu  =  %Lf  = %4d:%02d:%02d at %02d:%02d:%9Lf\n",
          Last_Time_Coarse, Last_Time_Fine, IntegerTime_from_CoarseFine ( Last_Time_Coarse, Last_Time_Fine ),
          JulianDay, year, month, day, hours, minutes, seconds );
      fprintf ( ptr_to_SummaryFile,
          "\nLast time in the TTE data:\n%11u : %6u  =  %20llu  =  %Lf  = %4d:%02d:%02d at %02d:%02d:%9Lf\n",
          Last_Time_Coarse, Last_Time_Fine, IntegerTime_from_CoarseFine ( Last_Time_Coarse, Last_Time_Fine ),
          JulianDay, year, month, day, hours, minutes, seconds );


      //  Useful summary info to screen and summary file;
      //  esp. important output to the summary file for use by the next program: which files were output:

      Detector_Output_Cnt = 0;
      for ( j_det=0;  j_det < NUM_DET;  j_det++ ) {
         if ( DetectorFiles [j_det] .DetectorFile_Opened )  Detector_Output_Cnt ++;
      }

      printf ( "\nTTE Data was found for %3u Detectors.\n", Detector_Output_Cnt );
      for ( j_det=0;  j_det < NUM_DET;  j_det++ )
         printf ( "Output %u events for detector %2u.\n", TTE_count_by_det [j_det], j_det );

      fprintf ( ptr_to_AnalysisFile, "\n%3u Detectors had TTE data.\n\n", Detector_Output_Cnt );
      fprintf ( ptr_to_SummaryFile, "\n%3u Detectors had TTE data.\n\n", Detector_Output_Cnt );


      for ( j_det=0;  j_det < NUM_DET;  j_det++ ) {
         if ( DetectorFiles [j_det] .DetectorFile_Opened ) {
            fprintf ( ptr_to_AnalysisFile, "%6u   %s\n", j_det, DetectorFiles [j_det] .DetectorFileName_ptr );
            fprintf ( ptr_to_SummaryFile, "%6u   %s\n", j_det, DetectorFiles [j_det] .DetectorFileName_ptr );
         }
      }

      Total_TTE_count = 0;
      fprintf ( ptr_to_AnalysisFile, "\nTable of number of TTE events.\nWARNING: data words before first time word are missing!\n" );
      fprintf ( ptr_to_SummaryFile, "\nTable of number of TTE events.\nWARNING: data words before first time word are missing!\n" );
      for ( j_det=0;  j_det < NUM_DET;  j_det++ ) {
         if ( DetectorFiles [j_det] .DetectorFile_Opened ) {
            Total_TTE_count += TTE_count_by_det [j_det];
            fprintf ( ptr_to_AnalysisFile, "Detector %2u: %u events output\n", j_det, TTE_count_by_det [j_det] );
            fprintf ( ptr_to_SummaryFile, "Detector %2u: %u events output\n", j_det, TTE_count_by_det [j_det] );
         }
      }  // j_det
      fprintf ( ptr_to_AnalysisFile, "\nTotal number of TTE events output: %llu\n\n", Total_TTE_count );
      fprintf ( ptr_to_SummaryFile, "\nTotal number of TTE events output: %llu\n\n", Total_TTE_count );
      printf ( "\nTotal number of TTE events output: %llu\n\n", Total_TTE_count );

   } else {  // Closeout / Normal case ?



      //  ***** Normal case: process the TTE words
      //  (indicated by a realistic value for Number_TTE_DataWords)


      for ( i_tte=0;  i_tte < Number_TTE_DataWords;  i_tte++ ) {


         this_det = TTE_Detector [i_tte];       // create a shorter name


         //  A secondary purpose -- record information to output summary
         //  at the end of the run (i.e., at closeout).

         //  Tabulate number of TTE events by detector:

         TTE_count_by_det [this_det] ++;


         //  Latch the earliest and last times in the data stream.
         //  We can't simply latch the first and last times encountered-- since the
         //  data is read from the detectors by the DPU hardware in round robin fashion,
         //  across detectors, the first word from the first detector may not have the
         //  earliest time -- maybe the first time is from a different detector.
         //  Similarly for the last time.
         //  So use a very simple technique, brute force approach of simply comparing
         //  every TTE time with the currently latched first/last values.

         //  Use GBM MET Time merged into a single long long unsigned int (aka GBM Unified Time)
         //  to do the comparison in an simple manner without any risk of rounding problems.

         ThisTime = IntegerTime_from_CoarseFine ( TTE_CoarseTime [i_tte], TTE_FineTime [i_tte] );

         if ( ThisTime < First_CombinedTime ) {  // earlier time found ?

            First_CombinedTime = ThisTime;
            First_Time_Coarse = TTE_CoarseTime [i_tte];
            First_Time_Fine = TTE_FineTime [i_tte];

         }  // earlier time found ?

         if ( ThisTime > Last_CombinedTime ) {  // later time found ?

            Last_CombinedTime = ThisTime;
            Last_Time_Coarse = TTE_CoarseTime [i_tte];
            Last_Time_Fine = TTE_FineTime [i_tte];

         }  // later time found



         //  Open the file for a detector the first time we encounter data
         //  for that detector.   This way we never create data files for
         //  detectors that aren't present in the input data stream.


         if ( ! DetectorFiles [this_det] .DetectorFile_Opened ) {  // need to open output file ?

            DetectorFiles [this_det] .ptr_to_DetectorFile =
                    fopen ( DetectorFiles [this_det] .DetectorFileName_ptr, "w" );

            if ( DetectorFiles [this_det] .ptr_to_DetectorFile == NULL ) {
               printf ( "\n\nFailed to open TTE output File for det %u!  Exiting!\n", this_det );
               exit (2);
            } else {

               printf ( "Opened TTE output file for det %u\n", this_det );
               DetectorFiles [this_det] .DetectorFile_Opened = true;
            }

         }  // need to open output file ?


         //  Primary action of this routine -- output the TTE data
         //  to files, one file per detector.

         TTE_Data.CoarseTime = TTE_CoarseTime [i_tte];
         TTE_Data.FineTime = TTE_FineTime [i_tte];
         TTE_Data.SpecChannel = TTE_Channel [i_tte];


         num_written = fwrite ( &TTE_Data,
                                sizeof ( TTE_Data ),
                                1,
                                DetectorFiles [this_det] .ptr_to_DetectorFile );

         if ( num_written != 1 ) {

            printf ( "\n\nOutput of TTE Fails.  Exiting.\n" );
            exit (3);

         }


      }  // i_tte loop


   }  // Closeout / Normal case ?


   return;

}  // Output_TTE ()
