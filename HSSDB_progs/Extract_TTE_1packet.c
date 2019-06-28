
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.

//  Revised by MSB 2014 April 17.

//  Revised by MSB 2014 March 16.  Compile-time options ALLOW_ERR_ONE and
//  ALLOW_ERR_TWO suppress correction of TTE errors of types one and two.

//  Michael S. Briggs, 2007 September 18 -- October 8,  UAH / NSSTC / GBM.


//  Adapted from my program FileScan, which summarizes the content of GBM HSSDB files.

//  This routine receives as input the packet data (excluding header) of a TTE packet and
//  returns the TTE events to the calling routine.  It takes a simpler approach to processing
//  TTE than the routine ProcessTTE.c -- even though this routine process the TTE one packet
//  at a time, the approach of this routine is more "stream" oriented, with less emphasis
//  placed on understanding the individual packets.


// ??? TO BE FIXED:
//  Note: Data Words before the first TTE Time Words are skipped, and
//  aren't included in the value TTE_DataWords_Output !

//   WARNING ???  Currently I am assuming that the TTE is contiguous ????
//                E.g., can it handle the FIFO dump ?



//  This routine returns the TTE data of the input packet to the calling routine.
//  The data is "cleaned" and simplified.   A simplification is instead of having a mixture
//  of TTE Time Words and TTE Data Words with only the fine time, the output of the routine
//  is the full / simple parameters for each TTE Event: GBM Coarse Time, GBM Fine Time,
//  (energy) Channel and Detector.

//  Furthermore time is cleaned by correcting for the FPGA timing errors: the jumps forward and
//  back by 0.1 s, and the omission of the TTE time word.

//  This routine checks for bad data and suspicious data patterns ("anomalies") and
//  outputs error and warning messages.

//  If this routine didn't have to correct time errors, and if it omitted the checking for
//  anomalies, it would be much simpler.   One reason that the anomaly section is complicated
//  is that a buffer is maintained so that events before the anomaly can be output.   Events
//  before and after a suspected anomaly are output so that a human can analyze the suspected
//  anomaly.


//  TTE formats:

//  format of TTE Time word:

//  bits  0 to 27: least sig 28 bits of the 32 bit coarse time
//  bits 28 to 31: 0xF to identify the word as a TTE Time Word

//  format of TTE Data word:

//  bits  0 to  6: channel
//  bit         7: unused
//  bits  8 to 11: det
//  bits 12 to 15: unused
//  bits 16 to 31: all 16 bits of the fine time



#include "HSSDB_Progs_Header.h"

#include <math.h>
#include <limits.h>
#include <stdint.h>


//  The next two parameters control how many events are output before and after
//  a suspected anomaly:

//   was BUFFER_DEPTH  1500  &  POST_ANOMALY_OUTPUT  500

#define  BUFFER_DEPTH  250
#define  POST_ANOMALY_OUTPUT  100

#define  LINE_LENGTH    100

typedef struct  BufferEntry_type {

   char Line [LINE_LENGTH];
   _Bool  EntryUsed;

}  BufferEntry_type;



//  The file-scope variables and additional routines of this file are all related to the Buffer.
//  The Buffer acts as a ring buffer, storing past events.   If a suspected anomaly is
//  detected, the buffer is dumped.  This allows a person to study the anomaly.

//  Variables shared between the routines of this file, but local to this file:

static  BufferEntry_type  TTE_Buffer [BUFFER_DEPTH];

static  uint32_t  BufferPosition = 0;


//  Local subroutine / function prototypes:

void  Insert_into_Buffer ( char Line [LINE_LENGTH] );

void  DumpBuffer ( FILE * ptr_to_SummaryFile );

void  BufferErase ( void );


//   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***


void Extract_TTE_1packet (

   // Input arguments:

   uint32_t PacketData [],
   uint16_t PacketDataLength,
   uint32_t HeaderCoarseTime,
   FILE * ptr_to_SummaryFile,

   // Input/Ouput arguments:

   uint32_t  ErrorCounts [NUM_ERROR_TYPES],

   // Output arguments:

   uint16_t * Number_TTE_DataWords_ptr,
   uint32_t TTE_CoarseTime [],
   uint16_t TTE_FineTime [],
   uint16_t TTE_Channel [],
   uint16_t TTE_Detector []

) {

   //  ----------------------------------------------------------------------------------

   //  These variables have to be "static" because this routine is called once per packet,
   //  but these variables need to retain information across packets, and thus across calls
   //  to this routine.

   //  MostRecentTTE_TimeWord records the most recent TTE Time Word, including across
   //  packets if needed.   The initial value is an illegal value used to mark that the
   //  value is invalid -- the most signficant nibble of a TTE Time Word must be 0xF.

   static  uint32_t  MostRecentTTE_TimeWord = 0x0;    // illegal value
   static  uint32_t  Previous_MostRecentTTE_TimeWord = 0x0;

   static  uint16_t  Previous_FineTime = UINT16_MAX;   // illegal value

   static  _Bool  JustRead_TTE_TimeWord = false;

   static  _Bool   Last_TTE_TimeWord_ErrorOne = false;

   static _Bool  First_Time = true;


   // when non-zero, write current event to anomaly file:
   static  uint32_t  PostAnomalyCounter = 0;

   static  long double  TTE_Time_by_Det [NUM_DET];
   static  _Bool  Have_TTE_Time_by_Det [NUM_DET] =
      { false, false, false, false, false, false, false,
        false, false, false, false, false, false, false };

   //  ----------------------------------------------------------------------------------


   uint32_t  FullCoarseTime;
   uint32_t  Temp_CoarseTime;

   uint16_t chan;
   uint16_t det;
   uint16_t FineTime;

   uint32_t i_word;

   uint32_t  Word4;
   uint8_t LeadingNibble;

   uint16_t i_tte = 0;

   static long double  Previous_TTE_Time = 0.0L;
   long double  TTE_Time;
   long double  TimeDiff;
   long double  TimeDiff_to_be_Anomaly = 0.005L;    // was using 0.010L for a long time

   char Line [LINE_LENGTH];

   _Bool  Found_Anomaly_One = false;
   _Bool  Found_Anomaly_Two = false;

   char Anomaly_String_One [200], Anomaly_String_Two [200], Anomaly_String_Three [200];


   //  ----------------------------------------------------------------------------------


   //  Entering this routine ==> new TTE packet.
   //  Announcements to screen, Buffer  (if currently outputting to that file)
   //  of the packet boundary:

   sprintf ( Line, "\n\nNew TTE Packet\n" );
   //fprintf ( ptr_to_SummaryFile, "%s", Line );
   Insert_into_Buffer ( Line );
   if ( PostAnomalyCounter > 0 )  fprintf ( ptr_to_SummaryFile, "\n\nNew TTE Packet\n" );


   *Number_TTE_DataWords_ptr = 0;


   //  loop over the words in the TTE Packet that was input to this routine in this call:

   for ( i_word=0; i_word < PacketDataLength / 4; i_word++ ) {  //  loop over TTE words


      //  The TTE data natively consists of 4-byte big-endian words.
      //  Because the TTE data was read in 4-byte words on an Intel processor, which is
      //  little-endian, it needs to be byte-swapped.

      Word4 = ByteSwap_4 ( PacketData [ i_word ] );


       //  Identify whether this TTE Word is a TTE Time Word or a TTE Data Word by the value
       //  of its Leading (i.e., most-significant) Nibble.    The most-significant nibble of
       //  a TTE Time Word must be 0xF, while the most-significant nibble of a TTE Data Word,
       //  can never be >= 0xD because the most-significant 16 bits are the fine-time field and
       //  hence can never be larger than 50,000 = 0xC350.


      LeadingNibble = (unsigned char)  ( Word4 >> 28 );

      if ( LeadingNibble == 0xF ) {  //  Time or Data Word ?

         //  THIS IS A TIME WORD:

         //  the least-significant 28 bits is the time field, so we copy only that portion.

         MostRecentTTE_TimeWord = Word4  &  0x0FFFFFFF;

         JustRead_TTE_TimeWord = true;

         // various output: summary, Buffer for possible use, anomaly file after anomaly:
         sprintf ( Line, "\nRead TTE Time Word: %u  %u\n", MostRecentTTE_TimeWord,
            ( HeaderCoarseTime & 0xF0000000 )  |  MostRecentTTE_TimeWord );
         Insert_into_Buffer ( Line );
         if ( PostAnomalyCounter > 0 )  fprintf ( ptr_to_SummaryFile, "\nAnomaly output: Read TTE Time Word: %u  %u\n", MostRecentTTE_TimeWord,
            ( HeaderCoarseTime & 0xF0000000 )  |  MostRecentTTE_TimeWord );


         //  Speculation of an error that might occur -- what if we insert a Coarse Time word
         //  because we detect the roll over of the fine time, but then shortly thereafter
         //  we encounter an actual Coarse Time Word, a bit after the roll over.  Probably
         //  in this case the inserted and actual Coarse Time Words will have the same values
         //  and so there will be no consequences.   But let's check whether this happens:

         if ( Last_TTE_TimeWord_ErrorOne  &&  Previous_FineTime < 5000 ) {

            ErrorCounts [5]++;
            printf ( "\nError type 5 with unexpected Time Word: %u %u\n", Previous_MostRecentTTE_TimeWord, MostRecentTTE_TimeWord );
           fprintf (ptr_to_SummaryFile,  "\nError type 5 with unexpected Time Word: %u %u\n", Previous_MostRecentTTE_TimeWord, MostRecentTTE_TimeWord );

         }

         Last_TTE_TimeWord_ErrorOne = false;


      } else {  //  Time or Data Word ?

         //  THIS IS A DATA WORD:

         //  Now extract the data from the Data Word.

         chan = Word4 & 0x7F;
         det = (Word4 >> 8) & 0xF;

         FineTime = (uint16_t)  ( Word4 >> 16 );


         //  Only process the DATA WORD if the contents are valid -- invalid contents
         //  mean that this program is confused in some way, or the file is corrupted --
         //  trying to use the bad data might crash the program, e.g., "det" is used as an array index.

         if ( det >= 14  ||  chan >= NUM_SPEC_CHAN  || FineTime >= 50000 ) {  // data valid ?

            printf ( "\n\n" );
           fprintf (ptr_to_SummaryFile,  "\n\n" );

            if ( det >= 14 ) {
                printf ( " ############  Bad Detector Value: %u\n", det );
               fprintf (ptr_to_SummaryFile, " ############  Bad Detector Value: %u\n", det );
            }     // valid det num ?

            if ( chan >= NUM_SPEC_CHAN ) {
                printf ( " ############  Bad Channel Value: %u\n", chan );
               fprintf (ptr_to_SummaryFile, " ############  Bad Channel Value: %u\n", chan );
            }     // valid channel ?

            if ( FineTime >= 50000 ) {
                printf ( " ############  Bad Fine Time Value: %u\n", FineTime );
               fprintf (ptr_to_SummaryFile, " ############  Bad Fine Time Value: %u\n", FineTime );
            }     // valid channel ?

            printf ( " ############  SKIPPING this bad DATA WORD ! ############\n\n" );
            fprintf (ptr_to_SummaryFile, " ############  SKIPPING this bad DATA WORD ! ############\n\n" );

         } else {  // data valid ?


            //  We are processing a data word, but we can only decode the time of the data word
            //  if we have previously encountered a TTE Time Word (either on this call / packet
            //  or on a previous call / packet).   We need the TTE Time Word to assemble the
            //  coarse time.
            //  If MostRecentTTE_TimeWord has the value 0x0, then we not yet encountered a
            //  TTE Time Word.
            //  ????   THIS WILL BE FIXED

            if ( MostRecentTTE_TimeWord  !=  0x0 ) {   //  have TTE Time Word ?


               //  OK, we previously encountered a TTE Time Word, so we can process this Data Word.

               //  But first, check for and correct for FPGA Time Errors.
               //  See FPGA_memo.txt, by Michael Briggs, with contributions from Narayana Bhat, 2005
               //  January 17, [memo on firmware bugs with the METs and PPS synchronization]


               //  Check for and, if found, correct FGPA Time Error of Type 1:
               //  But did the FPGA skip a TTE Time Word?
               //  This is detected by the Fine Time Rolling over without the TTE Time Word incrementing just before.
               //  When the Fine Time Counter Rolls over, it changes from a large value, near 50,000, to a small value,
               //  near zero.   We can't just test for a value inversion, merely later value < earlier value, because
               //  the TTE events are read round robin, so events from higher detector numbers may be slightly earlier
               //  than events already read from lower detector number.   The minimum rate of TTE in flight, even the background
               //  rate from one detector, will be sufficiently high that in between TTE Time Words there will always be
               //  many data words, and so, considering each group of data words in between TTE Time words: the first
               //  one should always have a fine time below 5000, and the last one a fine time above 50,000 - 5000.
               //  A characteristic of this error, but which is not used to detect this error: An omitted TTE Time Word only
               //  occurs when the missing TTE Time Word is a multiple of ten.
               //  Documentation: memo by Michael Briggs and Narayana Bhat, 2005 January 17, sections 6 & 12 (Step 1).

               if ( FineTime < Previous_FineTime  &&  Previous_FineTime > 45000  &&  FineTime < 5000 ) { // FineTime rollover ?

                  //  A roll over of the fine time counter just occured -- this means that the coarse time counter should
                  //  have incremented, which means that the preceding word should have been a TTE time word -- was it,
                  //  or was it missing?  -- a common error pattern.

                  if ( ! JustRead_TTE_TimeWord ) {  // missing TTE Time Word ?

                    //  Missing TTE Time Word detected -- an Error of Type 1 has been detected:

                    ErrorCounts [1]++;
                    Last_TTE_TimeWord_ErrorOne = true;

 						  //  Implement the correction: create the missing value:

						  MostRecentTTE_TimeWord++;

                    // various output: summary, Buffer for possible use, anomaly file after anomaly:
                    sprintf ( Line,
                              "\n\nError Type 1: Missing TTE Time Word -- creating the missing value: %u\n", MostRecentTTE_TimeWord );
                    fprintf ( ptr_to_SummaryFile, "%s", Line );
                    fprintf ( ptr_to_SummaryFile, "count of type 1 errors: %u\n", ErrorCounts [1] );
                    Insert_into_Buffer ( Line );
                    if ( PostAnomalyCounter > 0 )  fprintf ( ptr_to_SummaryFile,
                              "\n\nAnomaly output: Missing TTE Time Word -- creating the missing value: %u\n", MostRecentTTE_TimeWord );


                     //  Test whether the omitted TTE Time Word has an expected value, i.e., the missing value is a
                     //  multiple of ten.    Also occasionally observed is one more than a multiple of ten.
                     //  Other values are very rarely seen.
                     //  See below for the creation of the full coarse time.

                     Temp_CoarseTime = ( HeaderCoarseTime & 0xF0000000 )  |  MostRecentTTE_TimeWord;
                     if ( Temp_CoarseTime % 10 != 0  &&  Temp_CoarseTime % 10 != 1 ) {

                       // various output: summary, Buffer for possible use, anomaly file after anomaly:
                       sprintf ( Line,
                             "\nThe Missing TTE Time Word has rare value: %u, %u\n", MostRecentTTE_TimeWord, Temp_CoarseTime );
                       fprintf ( ptr_to_SummaryFile, "%s", Line );
                       Insert_into_Buffer ( Line );
                       if ( PostAnomalyCounter > 0 )  fprintf ( ptr_to_SummaryFile,
                             "\nThe Missing TTE Time Word has rare value: %u, %u\n", MostRecentTTE_TimeWord, Temp_CoarseTime );

                     }

                  }  // missing TTE Time Word ?


               } // FineTime rollover ?


               Previous_FineTime = FineTime;   // we are done with Previous Fine Time for this data word, so update


               //  We just read a TTE Data Word, so we didn't just read a TTE Time Word;
               //  we couldn't update this variable until this location because of its use above in a test.

               JustRead_TTE_TimeWord = false;



               //  Check for and, if found, correct FGPA Time Error of Type 2:
               //  If the TTE Coarse Time Word just advanced, did it advance by 2 instead of incrementing by 1?
               //  Normally MET Coarse Time increments every 0.1 s when the fine time rolls overs, except
               //  every 1 s, when instead it is set by the PPS.  But sometimes it does both -- the set takes place,
               //  which advances the MET Coarse counter by 1, and it also increments, which also advances the
               //  MET Coarse counter by 1, for a net incorrect increment of 2.   The cause is a hypothesis, but observations
               //  show incorrect advances by 2 sometimes take place when the MET Coarse Time is a multiple of ten.
               //  This is a condition of the MET counters effecting all GBM Times, for example, this condition should
               //  always be associated with FSW WARNING message 13084, created by the FSW by
               //  by analyzing the ITIME data.   Furthermore, this condition is created at a PPS signal --
               //  the MET counters will have the wrong time until the next PPS signal and so ten TTE Time Words in a row
               //  will be wrong.
               //  Documentation: memo by Michael Briggs and Narayana Bhat, 2005 January 17, sections 3--5, 12 (Step 2).

               if ( MostRecentTTE_TimeWord == Previous_MostRecentTTE_TimeWord + 2 ) {

                  // Yes, an Error of Type 2 has occured:

                  ErrorCounts [2]++;
                  Last_TTE_TimeWord_ErrorOne = false;

                  // various output: summary, Buffer for possible use, anomaly file after anomaly:
                  sprintf ( Line,
                           "\n\nError Type 2: Will correct TTE Time Word that advanced by 2. Wrong value = %u\n", MostRecentTTE_TimeWord );
                  fprintf ( ptr_to_SummaryFile, "%s", Line );
                  Insert_into_Buffer ( Line );
                  if ( PostAnomalyCounter > 0 )  fprintf ( ptr_to_SummaryFile,
                           "\n\nError Type 2: Will correct TTE Time Word that advanced by 2. Wrong value = %u\n", MostRecentTTE_TimeWord );

                  //  The fix for a Type 2 Error: replace the bad coarse time value with the expected value:

                  MostRecentTTE_TimeWord = Previous_MostRecentTTE_TimeWord + 1;

               }

               Previous_MostRecentTTE_TimeWord = MostRecentTTE_TimeWord;



               //  All known types of time errors have been corrected by the above code.
               //  Now assemble the TTE time data from three data items into the standard GBM
               //  time form of ( Coarse Time, Fine Time ).

               //  The full coarse time is obtained by combining bits from two sources:
               //  transferring the four MSB from the header coarse time into the four MSB of of the coarse
               //  time and transferrring the least-significant 28 bits of the TTE Time Word as the 28 LSB
               //  of the coarse time -- we only transferred the least signficant 28 bits from the data to
               //  MostRecentTTE_TimeWord, so no further masking is needed here on that item.
               //  Also see Time_from_TTE_Data.

               //  The Fine Time is easy -- the TTE Data Words contain the full Fine Time, which is
               //  simply copied to the output.

               FullCoarseTime = ( HeaderCoarseTime & 0xF0000000 )  |  MostRecentTTE_TimeWord;

               //  array goes from 0 to MAX_TTE_WORDS_PER_PACKET - 1
               if ( i_tte <= MAX_TTE_WORDS_PER_PACKET-1 ) {  // space in output arrays ?

                  TTE_CoarseTime [i_tte] = FullCoarseTime;
                  TTE_FineTime [i_tte] = FineTime;
                  TTE_Channel [i_tte] = chan;
                  TTE_Detector [i_tte] = det;


                  // various output: summary, Buffer for possible use, anomaly file after anomaly:

                  sprintf ( Line, "%10u  %5u  %2u  %3u\n",
                             TTE_CoarseTime [i_tte],
                             TTE_FineTime [i_tte],
                             TTE_Detector [i_tte],
                             TTE_Channel [i_tte] );

                  Insert_into_Buffer ( Line );

                  if ( PostAnomalyCounter > 0 )  {  // output current event to anomaly file ?

                     fprintf ( ptr_to_SummaryFile, "%10u  %5u  %2u  %3u\n",
                             TTE_CoarseTime [i_tte],
                             TTE_FineTime [i_tte],
                             TTE_Detector [i_tte],
                             TTE_Channel [i_tte] );

                     //  The basic purpose of PostAnomalyCounter is to count the number
                     //  of TTE data words output post an Anomaly.   The number of lines
                     //  output might be slightly greater because certain other events
                     //  are also output when the counter is non-zero.

                     PostAnomalyCounter--;

                     if ( PostAnomalyCounter == 0 )  fprintf ( ptr_to_SummaryFile, "\nEnd of post-Anomaly output.\n\n" );

                  }  // output current event to anomaly file ?


                  i_tte++;   //  count only words that will be output



                  //  Check for anomalies in the stream of TTE data, such as a significant jump backwards,
                  //  more than a single event out of order, or a jump forward, by more than the expected gap
                  //  between events.   Slight time jumps backwards are expected, as explained above,
                  //  when comparing times of events from different detectors, because the detectors are
                  //  read round robin.   Anomalies will cause output for human examination to determine
                  //  whether there really is a problem.
                  //  Obviously can only check for a time jump if have a previous time.
                  //  Also suppress this check if we are currently outputting events after a previous
                  //  detected anomaly.

                  TTE_Time = FloatTime_from_CoarseFine ( FullCoarseTime, FineTime );

                  if ( !First_Time  && PostAnomalyCounter == 0 ) {  // check for gross anomaly ?

                     TimeDiff = TTE_Time - Previous_TTE_Time;

                     if ( TimeDiff < -TimeDiff_to_be_Anomaly  ||  TimeDiff > TimeDiff_to_be_Anomaly ) {  // time jump ?

                        //   A time anomaly has been detected !!!

                        Found_Anomaly_One = true;

                        sprintf ( Anomaly_String_One,
                               "Gross Time anomaly detected!  Previous, Current: %Lf  %Lf", Previous_TTE_Time, TTE_Time );

                       sprintf ( Anomaly_String_Two,
                                 "Gross Time anomaly Detected Here!  Previous, Current: %Lf  %Lf", Previous_TTE_Time, TTE_Time );
                       strncpy ( Anomaly_String_Three, " ", 2 );

                     }  // time jump ?

                  }  // check for gross anomaly ?


                  First_Time = false;
                  Previous_TTE_Time = TTE_Time;



                  //  As explained above, when compared across all TTE fine time words, slight time reversals are
                  //  expected, because the TTE data is read round robin.  A TTE event read from a high detector number
                  //  may wait to be read while earlier events are read and therefore could be earlier by microseconds
                  //  than events from smaller detector numbers.  So the above test for timing anomalies accepted
                  //  small time jumps backwards.   But there should be no time reversals considering the data from
                  //  each detector, excluding the data from the other detectors.

                  //  But don't check if we have just detected a "gross" time anomaly.
                  //  Also suppress this check if we are currently outputting events after a previous
                  //  detected anomaly.

                  if ( Found_Anomaly_One == false  && PostAnomalyCounter == 0 ) {  // not already anomaly ?

                     if ( Have_TTE_Time_by_Det [det] ) {

                        if ( TTE_Time < TTE_Time_by_Det [det] ) {

                           //  Type 2 time anomaly detected -- time reversal for the data of one detector !

                           Found_Anomaly_Two = true;

                           sprintf ( Anomaly_String_One, "Time Anomaly Detected -- reversed time for detector %u.", det );
                           sprintf ( Anomaly_String_Two, "Time Anomaly Detected HERE -- reversed time for detector %u.", det );
                           sprintf ( Anomaly_String_Three, "Previous, Current: %Lf  %Lf", TTE_Time_by_Det [det], TTE_Time );

                        }

                     }

                  }  // not already anomaly ?


                  //  Have used these variables as telling us about the previous data,
                  //  now done using them for this event, so can update:

                  Have_TTE_Time_by_Det [det] = true;
                  TTE_Time_by_Det [det] = TTE_Time;



                  //   If either type of anomaly just detected, make the anomaly output:

                  if ( Found_Anomaly_One || Found_Anomaly_Two ) {   // make anomaly output ?

                    //  Output the pre-anomaly buffer, and set the counter so that post-anomaly output will occur.

                    printf ( "\n%s\n", Anomaly_String_One );
                    fprintf ( ptr_to_SummaryFile, "\n%s\n", Anomaly_String_One );

                    DumpBuffer ( ptr_to_SummaryFile );

                    fprintf ( ptr_to_SummaryFile, "\n%s\n", Anomaly_String_Two );
                    fprintf ( ptr_to_SummaryFile, "\n%s\n", Anomaly_String_Three );

                    PostAnomalyCounter = POST_ANOMALY_OUTPUT;


                    Found_Anomaly_One = false;   // don't make output twice!
                    Found_Anomaly_Two = false;


                  }   // make anomaly output ?



               } else {  // space in output arrays ?

                   printf ( "\n\nExtractTTE: out of space in output arrays: %u %u\n", i_tte, MAX_TTE_WORDS_PER_PACKET );
                  fprintf ( ptr_to_SummaryFile, "\n\nExtractTTE: out of space in output arrays: %u %u\n",
                                                                                      i_tte, MAX_TTE_WORDS_PER_PACKET );

               }  // space in output arrays ?


            }   //  have TTE Time Word ?

         }  // data valid ?

      }  //  Time or Data Word ?

   }  //  loop over TTE words


   *Number_TTE_DataWords_ptr = i_tte;


}  //  Extract_TTE_1packet ()



//   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***


void  Insert_into_Buffer (

   char Line [LINE_LENGTH]

) {

   static  _Bool  BufferNeverUsed = true;

   //  ----------------------------------------------------------------------------------


   if ( BufferNeverUsed ) {  // need to init Buffer?

      BufferNeverUsed = false;

      BufferErase ();

   }  // need to init Buffer?


   //  The normal functions of this routine:
   //  1) insert the Line input to this routine into the Buffer, overwriting
   //  the previously oldest entry.
   //  2) Mark this entry as used / initialized.
   //  3) Update the pointer to the oldest entry.

   strncpy ( TTE_Buffer [BufferPosition] .Line, Line, LINE_LENGTH );

   TTE_Buffer [BufferPosition] .EntryUsed = true;

   BufferPosition++;
   if ( BufferPosition == BUFFER_DEPTH )  BufferPosition = 0;


}  // Insert_into_Buffer ();




//   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***


//  This routine outputs the contents of the Buffer.
//  The Buffer is a ring buffer, so this routine has to unwrap it, which
//  is why it is output in two segments.
//  Since the Buffer may not be full, it checks to see whether each slot of
//  the Buffer is actually in use.


void  DumpBuffer (

   FILE * ptr_to_AnomalyFile

) {

   unsigned int i;

   // ---------------------------------------


   fprintf ( ptr_to_AnomalyFile, "Begin Dump of up to %u events before the Anomaly...\n", BUFFER_DEPTH );


   //  first segment:

   for ( i=BufferPosition;  i<BUFFER_DEPTH;  i++ ) {

      if ( TTE_Buffer [i] .EntryUsed )  fprintf ( ptr_to_AnomalyFile, "%s", TTE_Buffer [i] .Line );

   }


   //   second segment:

   for ( i=0;  i<BufferPosition;  i++ ) {

      if ( TTE_Buffer [i] .EntryUsed )  fprintf ( ptr_to_AnomalyFile, "%s", TTE_Buffer [i] .Line );

   }


   fprintf ( ptr_to_AnomalyFile, "\nEnd of dump of events before the Anomaly\n\n" );



   //  Buffer has been dumped, now erase it:

   BufferErase ();


}  // DumpBuffer ();




//   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***


//  This routine initializes / erases the Buffer.
//  Each entry is marked as unused / empty : EntryUsed = FALSE.
//  An additional action, which isn't strictly necessary: the contents of each
//  slot is erased.

void  BufferErase ( void ) {

   unsigned int i;

   // ---------------------------------------

   BufferPosition = 0;

   for ( i=0;  i<BUFFER_DEPTH;  i++ ) {

      TTE_Buffer [i] .Line [0] = '\0';
      TTE_Buffer [i] .EntryUsed = false;

   }

}  // Buffer Erase ();
