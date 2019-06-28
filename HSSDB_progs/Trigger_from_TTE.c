
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.


//  Michael S. Briggs, 2008 July 7 --
//  Work in progress.  Needs comments.  See AAA_DESCRIPTION.txt


#include "HSSDB_Progs_Header.h"

#include <limits.h>
#include <float.h>
#include <math.h>


#define  ONE_SEC_IN_TICKS   512000LLU     /* 1.024 s in 2 microsec ticks */
#define  BCKG_BUFFER_DEPTH   53U


#define  ONE_SEC   512000LLU  /* 1.024 s in 2 microsec ticks */

#define  MAX_ALLOWED_BCKG_AGE   18176000LLU   /* 35.5 * 1.024 s in 2 microsec ticks */

#define  MIN_ALLOWED_BCKG_GAP   2048000LLU    /* 4.096 s in 2 microsec ticks */

#define  MIN_REQ_GOOD_BCKG_BINS   30U

//  typedefs

typedef  struct  Background_Accum_type {
   _Bool  HaveData;
   uint32_t SummedCounts [NUM_NAI_DET];
   uint32_t  BinNum;
   uint64_t  FirstDataTime;
   uint64_t  LastDataTime;
} Background_Accum_type;


//  filescope variables

uint64_t  BaseTime = UINT64_MAX;

uint64_t  Trigger_Timescale_in_Ticks;


//  local function prototypes:

uint64_t  LeftTimeBoundary_Bckg_Bin (
   uint32_t  BinNum
);

uint64_t  RightTimeBoundary_Bckg_Bin (
   uint32_t  BinNum
);

uint64_t  LeftTimeBoundary_Data_Bin (
   uint32_t  BinNum
);


int main ( void ) {


   uint32_t  Beg_Trigger_SPEC_units =  31;
   uint32_t  End_Trigger_SPEC_units =  83;


   FILE * Input_File_ptr;
   size_t num_read;

   uint64_t  TTE_events_count = 0;
   uint64_t  TTE_events_TriggerCriteria_count = 0;

   _Bool   First_Event = true;

   Processed_TTE_type  Processed_TTE;

   Background_Accum_type   Background_Ring_Buffer [BCKG_BUFFER_DEPTH];

   uint32_t  Oldest_BckBuffer_Slot = 0;

   uint32_t  Background_BinNum;
   uint32_t  BinNum_Bckg_Accum_Underway = 0;

   uint32_t  DataAccum_BinNum;
   uint32_t  BinNum_Data_Accum_Underway = 0;
   uint32_t  Prev_DataBinNum_with_Pos_Deviation = UINT32_MAX;

   uint64_t  FirstTime_of_NewBin = UINT64_MAX;
   uint64_t  MostRecentDataTime = UINT64_MAX;
   uint64_t  FirstDataTime_DataAccum = UINT64_MAX;

   uint32_t  j_det, this_det;

   uint32_t  i_ring;

   uint32_t  Current_Bckg_Accum [NUM_NAI_DET];
   uint32_t  Current_Data_Accum [NUM_NAI_DET];

   uint32_t  SummedCounts_for_Background [NUM_NAI_DET];
   double  BackgroundModel [NUM_NAI_DET];
   uint32_t  Num_Good_Bckg_Bins;

   uint64_t  BeginTime_Data_Bin;
   uint64_t  BackgroundAge;
   uint64_t  BackgroundGap;
   _Bool  Bckg_Bin_GOOD;

   uint32_t  DataAccum_with_Bckg = 0;
   uint32_t  DataAccum_without_Bckg = 0;

   double timescale_factor;
   double excess, sigma, sig_level_in_sigma;
   double  Largest_Pos_Deviation = -DBL_MAX;
   double  Largest_Neg_Deviation =  DBL_MAX;

   double ReportingThreshold;

   // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *



   printf ( " \nInput channel range to analyze in   >> SPEC <<   channels.\n" );
   printf ( "SPEC channels  0 to 127 for ITIME 0 to 7 for >10 keV\n" );
   printf ( "SPEC channels  8 to  30 for ITIME 1 to 2 for 10 to  50 keV\n" );
   printf ( "SPEC channels 18 to  30 for ITIME 2 to 2 for 25 to  50 keV\n" );
   printf ( "SPEC channels 31 to  83 for ITIME 3 to 4 for 50 to 300 keV\n" );
   printf ( "SPEC channels 50 to 127 for ITIME 4 to 7 for >100 keV\n" );
   printf ( "SPEC channels 84 to 127 for ITIME 5 to 7 for >300 keV\n" );
   printf ( "etc.\nInput SPEC channel range: " );
   scanf ( "%u%u", &Beg_Trigger_SPEC_units, &End_Trigger_SPEC_units );
   if ( Beg_Trigger_SPEC_units > End_Trigger_SPEC_units   ||
        Beg_Trigger_SPEC_units >= NUM_SPEC_CHAN  ||  End_Trigger_SPEC_units >= NUM_SPEC_CHAN ) {
      printf ( "\nIllegal input for SPEC channel range: %u to %u\n", Beg_Trigger_SPEC_units, End_Trigger_SPEC_units );
      exit (1);
   }

   printf ( "\n\nInput timescale to analyze in units of 2 microsecond 'ticks'.\n" );
   printf ( "   8000 for   16 ms\n" );
   printf ( "  32000 for   64 ms\n" );
   printf ( " 128000 for  256 ms\n" );
   printf ( " 512000 for  1.024 s\n" );
   printf ( "2048000 for  4.096 s\n" );
   printf ( "8192000 for 16.384 s\n" );
   printf ( "etc.\nInput Trigger timescale: " );
   scanf ( "%llu", &Trigger_Timescale_in_Ticks );
   if ( Trigger_Timescale_in_Ticks > 8192000 )  printf ( "\n\nAre you really sure that you want a timescale longer than 16.384 s ??\n\n" );


   printf ( "\n\nInput threshold, in sigma, for reporting deviations: " );
   scanf ( "%lf", &ReportingThreshold );


   printf ( "\n\n\nEnergy range for 'trigger' in SPEC channels: %u to %u\n", Beg_Trigger_SPEC_units, End_Trigger_SPEC_units );
   printf ( "Trigger timescale in 2 microsec 'tick' units: %llu\n", Trigger_Timescale_in_Ticks );
   printf ( "Trigger timescale in seconds: %f\n", Trigger_Timescale_in_Ticks * 2.0E-6 );
   printf ( "Threshold in sigma for reporting deviations: %5.2f\n", ReportingThreshold );



   for ( i_ring=0;  i_ring<BCKG_BUFFER_DEPTH;  i_ring++ )  Background_Ring_Buffer [i_ring] .HaveData = true;


   Input_File_ptr = fopen ( "Processed_TTE.dat", "r" );
   if ( Input_File_ptr == NULL ) {
      printf ( "\n\nFailed to open input TTE File -- Exiting!\n" );
      exit (1);
   }


   for ( j_det=0;  j_det<NUM_NAI_DET;  j_det++ )  Current_Bckg_Accum [j_det] = 0;


   while ( true ) {  // do "forever" -- process data

      num_read = fread ( &Processed_TTE, sizeof ( Processed_TTE ), 1, Input_File_ptr );
      if ( num_read != 1 ) {
         printf ( "\nNo more input data -- exiting !!\n" );

         if ( Largest_Pos_Deviation != -DBL_MAX ) {
            printf ( "\nLargest positive deviation found:  %7.2lf\n", Largest_Pos_Deviation );
         } else {
            printf ( "\nNo positive devations were found above %6.2lf sigma.\n", ReportingThreshold );
         }
         if ( Largest_Neg_Deviation !=  DBL_MAX ) {
            printf ( "Largest negative deviation found: %8.2lf\n", Largest_Neg_Deviation );
         } else {
            printf ( "No negative deviations were found exceeding %6.2lf sigma.\n", ReportingThreshold );
         }

         printf ( "\n%llu TTE events were processed.\n", TTE_events_count );
         printf ( "%llu of the TTE events were from NaI detectors within the trigger energy channels.\n", TTE_events_TriggerCriteria_count );
         printf ( "\nNumber of data accumulations with good background model: %u\n", DataAccum_with_Bckg );
         printf ( "Number of data accumulations without a good background model: %u\n", DataAccum_without_Bckg );

         return (0);                                //  <-------   NORMAL RETURN/EXIT FROM PROGRAM  !!!!!
      }

      TTE_events_count++;


      //  Only process events within the trigger energy range, and only process NaI detectors:

      if ( Processed_TTE.SpecChannel >= Beg_Trigger_SPEC_units  &&  Processed_TTE.SpecChannel <= End_Trigger_SPEC_units  &&
              Processed_TTE.Detector < NUM_NAI_DET ) {  // within trig E range ?

         TTE_events_TriggerCriteria_count++;

         this_det = Processed_TTE.Detector;   // create shorter variable name


         //  First TTE event in the file has the earliest time -- latch this time as the base time for
         //  calculating bin numbers:

         if ( First_Event ) {
            First_Event = false;
            BaseTime = Processed_TTE.Time_in_OneVariable;
            FirstTime_of_NewBin = BaseTime;   // special case for 1st bckg bin
            FirstDataTime_DataAccum = BaseTime;   // special case for 1st data bin
            printf ( "Earliest time in the file: %llu\n", BaseTime );
         }


         Background_BinNum = ( Processed_TTE.Time_in_OneVariable - BaseTime ) / ONE_SEC_IN_TICKS;


         //  Is the current background bin number, calculated from the time of the current TTE event, a new value?
         //  If so, much work to do re inserting the current background accumulation into the background
         //  ring buffer, etc.

         if ( Background_BinNum  !=  BinNum_Bckg_Accum_Underway ) {  // new background bin ?

            //  The current TTE event belongs to a new bin, so the accumulation underway is complete.
            //  Copy the accumulation into the Background Ring Buffer:

            Background_Ring_Buffer [Oldest_BckBuffer_Slot] .HaveData = true;

            for ( j_det=0;  j_det<NUM_NAI_DET;  j_det++ )
               Background_Ring_Buffer [Oldest_BckBuffer_Slot] . SummedCounts [j_det] = Current_Bckg_Accum [j_det];

            Background_Ring_Buffer [Oldest_BckBuffer_Slot] .BinNum = BinNum_Bckg_Accum_Underway;

            Background_Ring_Buffer [Oldest_BckBuffer_Slot] .FirstDataTime = FirstTime_of_NewBin;
            Background_Ring_Buffer [Oldest_BckBuffer_Slot] .LastDataTime = MostRecentDataTime;


            /*   debug output:
            printf ( "\nSlot %2lu, BN: %8lu DTR: %20llu -%20llu, BE: %20llu -%20llu,  diffs: %6llu, %6llu\n",
               Oldest_BckBuffer_Slot,
               Background_Ring_Buffer [Oldest_BckBuffer_Slot] .BinNum,
               Background_Ring_Buffer [Oldest_BckBuffer_Slot] .FirstDataTime,
               Background_Ring_Buffer [Oldest_BckBuffer_Slot] .LastDataTime,
               LeftTimeBoundary_Bckg_Bin ( Background_Ring_Buffer [Oldest_BckBuffer_Slot] .BinNum ),
               RightTimeBoundary_Bckg_Bin ( Background_Ring_Buffer [Oldest_BckBuffer_Slot] .BinNum ),
               Background_Ring_Buffer [Oldest_BckBuffer_Slot] .FirstDataTime -
                   LeftTimeBoundary_Bckg_Bin ( Background_Ring_Buffer [Oldest_BckBuffer_Slot] .BinNum ),
               RightTimeBoundary_Bckg_Bin ( Background_Ring_Buffer [Oldest_BckBuffer_Slot] .BinNum ) -
                   Background_Ring_Buffer [Oldest_BckBuffer_Slot] .LastDataTime );
            */

            //  Advance the pointer to the oldest slot in use in the Background Ring Buffer:

            Oldest_BckBuffer_Slot++;
            if ( Oldest_BckBuffer_Slot == BCKG_BUFFER_DEPTH )  Oldest_BckBuffer_Slot = 0;


            //  Background accumulation has been used, erase for new data;
            //  advance the current background bin number:

            for ( j_det=0;  j_det<NUM_NAI_DET;  j_det++ )  Current_Bckg_Accum [j_det] = 0;
            BinNum_Bckg_Accum_Underway = Background_BinNum;

            //  The current TTE event is the first event of the next bin, latch its time
            //  as the first time of that bin:

            FirstTime_of_NewBin = Processed_TTE.Time_in_OneVariable;

         } // new background bin ?


         //  This is the correct spot to latch the time of the current event.
         //  The last TTE event not to advance the background bin number will provide
         //  the value MostRecentDataTime to be the LastDataTime of the background bin.

         MostRecentDataTime = Processed_TTE.Time_in_OneVariable;


         //  Whether this is an old background bin, already with events, or just re-initialized
         //  becaused this event caused the background bin number to advance, this event
         //  needs to be accumulated.

         Current_Bckg_Accum [this_det] ++;



         //  Done with processing current event re background, updating background ring buffer, if needed, etc.


         DataAccum_BinNum = ( Processed_TTE.Time_in_OneVariable - BaseTime ) / Trigger_Timescale_in_Ticks;


         //  Is the current data bin number, calculated from the time of the current TTE event, a new value?
         //  If so, much work to do re comparing the data accumulation to the background model (i.e, running
         //  average of background accumulation), etc.

         if ( DataAccum_BinNum  !=  BinNum_Data_Accum_Underway ) {  // new data accum bin ?

            //  The current TTE event belongs to a new bin, so the accumulation underway is complete.


            /*  debug output
            printf ( "\nbin %10lu, 1st data: %20llu, left bin: %20llu, diff: %6llu",
                BinNum_Data_Accum_Underway, FirstDataTime_DataAccum, LeftTimeBoundary_Data_Bin ( BinNum_Data_Accum_Underway ),
                FirstDataTime_DataAccum - LeftTimeBoundary_Data_Bin ( BinNum_Data_Accum_Underway ) );
            */


            //  Create a background model (i.e., running average) from the data in the Background Ring Buffer
            //  Just search the entire ring, checking whether 1) each slot has data, and 2) whether the data
            //  is at least 4 s before the the data accumulation and no more than 32 s before the data accumulation.
            //  No need to unwrap the ring buffer into correct time order since we examine all slots....

            Num_Good_Bckg_Bins = 0;
            for ( j_det=0;  j_det<NUM_NAI_DET;  j_det++ )  SummedCounts_for_Background [j_det] = 0;

            BeginTime_Data_Bin = LeftTimeBoundary_Data_Bin ( BinNum_Data_Accum_Underway );

            for ( i_ring=0;  i_ring<BCKG_BUFFER_DEPTH;  i_ring++ ) {

               if ( Background_Ring_Buffer [i_ring] .HaveData ) {  // slot has data?

                  Bckg_Bin_GOOD = true;

                  //  Right edge of background actually newer than left of data ?   Much too new!
                  //  How could this happen??   But it might cause problems with unsigned arithmetic with other calcs belows...
                  if ( RightTimeBoundary_Bckg_Bin ( Background_Ring_Buffer [i_ring] .BinNum )  >  BeginTime_Data_Bin )  Bckg_Bin_GOOD = false;

                  //  Is this background bin too old?
                  BackgroundAge = BeginTime_Data_Bin - LeftTimeBoundary_Bckg_Bin ( Background_Ring_Buffer [i_ring] .BinNum );
                  if ( BackgroundAge > MAX_ALLOWED_BCKG_AGE )  Bckg_Bin_GOOD = false;

                  //  Is this background bin too close in time to the data interval?
                  BackgroundGap = BeginTime_Data_Bin - RightTimeBoundary_Bckg_Bin ( Background_Ring_Buffer [i_ring] .BinNum );
                  if ( BackgroundGap < MIN_ALLOWED_BCKG_GAP )  Bckg_Bin_GOOD = false;

                  if ( Bckg_Bin_GOOD ) {  // Bckg bin passes tests?

                     Num_Good_Bckg_Bins ++;
                     for ( j_det=0;  j_det<NUM_NAI_DET;  j_det++ )
                        SummedCounts_for_Background [j_det] += Background_Ring_Buffer [i_ring] .SummedCounts [j_det];

                  }  // Bckg bin passes tests?


               }  // slot has data?

            }  // i_ring


            if ( Num_Good_Bckg_Bins < MIN_REQ_GOOD_BCKG_BINS ) {  // enough good background bins?

               DataAccum_without_Bckg ++;
               printf ( "\nOnly %u suitable bins for background accum!\n", Num_Good_Bckg_Bins );

            } else {  // enough good background bins?

               //  Calculate background model as average of accumulate counts -- switch to floating point (double):

               DataAccum_with_Bckg ++;
               for ( j_det=0;  j_det<NUM_NAI_DET;  j_det++ )
                   BackgroundModel [j_det] = (double) SummedCounts_for_Background [j_det] / (double) Num_Good_Bckg_Bins;

               //  Calculate differences between current data accumulations and background model:

               timescale_factor = (double) Trigger_Timescale_in_Ticks / (double) ONE_SEC;

               for ( j_det=0;  j_det<NUM_NAI_DET;  j_det++ ) {

                  excess = (double) Current_Data_Accum [j_det] - timescale_factor * BackgroundModel [j_det];
                  sigma = sqrt (timescale_factor * BackgroundModel [j_det]);

                  sig_level_in_sigma = excess / sigma;

                  //  Use fabs to look for positive and negative deviations !
                  if ( fabs (sig_level_in_sigma) > ReportingThreshold ) {  // significant deviation?

                      printf ( "det %2u has %6u counts, vs bckg %7.1f for %8.2f sigma in bin %10u at %20llu\n",
                      j_det, Current_Data_Accum [j_det], BackgroundModel [j_det], sig_level_in_sigma, BinNum_Data_Accum_Underway,
                      LeftTimeBoundary_Data_Bin (BinNum_Data_Accum_Underway) );

                      //  1) Latch most extreme positive and negative deviations seen in this datafile:
                      //  2) If positive excess, test whether this time bin number is the same as the previous
                      //     time bin number with a positive excess -- if so, TRIGGER!  = two detectors with excess in same time bin.
                      //  3) If positive excess, latch time bin number to compare on the next excess.

                      if ( sig_level_in_sigma > 0 ) {
                         if ( sig_level_in_sigma > Largest_Pos_Deviation )  Largest_Pos_Deviation = sig_level_in_sigma;
                         if ( BinNum_Data_Accum_Underway == Prev_DataBinNum_with_Pos_Deviation )  printf ( "Duplicate time bin ==> TRIGGER !!!\n" );
                         Prev_DataBinNum_with_Pos_Deviation = BinNum_Data_Accum_Underway;
                      } else {
                         if ( sig_level_in_sigma < Largest_Neg_Deviation )  Largest_Neg_Deviation = sig_level_in_sigma;
                      }

                  }  // significant deviation?

               }  // j_det

            }  // enough good background bins?



            //  Done procesing the current data accumulation -- erase for new data;
            //  advance the current background bin number:

            for ( j_det=0;  j_det<NUM_NAI_DET;  j_det++ )  Current_Data_Accum [j_det] = 0;
            BinNum_Data_Accum_Underway = DataAccum_BinNum;

            //  Latch the time of this TTE event as the first data time of the next bin:

            FirstDataTime_DataAccum = Processed_TTE.Time_in_OneVariable;


         }  // new data accum bin ?


         //  Whether we accumulate this event into an "old" data accumulation, or we just re-initialized
         //  the data accumulation because this event caused the data bin number to advance,
         //  this event needs to be accumulated:

         Current_Data_Accum [this_det] ++;



      }  // within trig E range ?



   }  // do "forever" -- process data


}  // main ()



//  **********************************************************************************************************************

uint64_t  LeftTimeBoundary_Bckg_Bin (
   uint32_t  BinNum
) {

   return ( BaseTime + ONE_SEC_IN_TICKS * BinNum );

}  // LeftTimeBoundary_Bckg_Bin ()


//  **********************************************************************************************************************

uint64_t  RightTimeBoundary_Bckg_Bin (
   uint32_t  BinNum
) {

   return ( BaseTime + ONE_SEC_IN_TICKS * (BinNum + 1ULL) - 1ULL );

}  // RightTimeBoundary_Bckg_Bin ()




//  **********************************************************************************************************************

uint64_t  LeftTimeBoundary_Data_Bin (
   uint32_t  BinNum
) {

   return ( BaseTime + Trigger_Timescale_in_Ticks * BinNum );

}  // LeftTimeBoundary_Data_Bin ()
