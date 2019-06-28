
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.


//  Michael S. Briggs, 2004 Oct 8, UAH / NSSTC.
//  Revised 2007 March 13: output corrected coarse time; also explicitly declare ints to be long ints
//  for platforms / compilers in which that isn't the default.

//  MSB, 2007 Sept 19: This subroutine is currently obsolete for two reasons:

//  1) It is overly complicated for most purposes:
//  This subroutine attempts to identify the time range of the data words of each
//  TTE packet.  This is much more complicated than just merging all TTE data
//  of a "chunk" together and calculating the time of each data word.   TTE is
//  much easier to process, if unlike this subroutine, you don't try to understand
//  it at the packet level and instead just merge it into a stream.  The only reason
//  to look at packets is to recognize when you have a different TTE "chunk", e.g.,
//  switching from prompt TTE in a trigger to the pre-trigger FIFO readout.

//  2) It doesn't correct timing errors that are created by the FPGAs.


//  Assumption:
//  The 4 most signficant bits of the coarse time of any TTE packet in the file
//  apply to all TTE words in the packet-- even to TTE words from a FIFO dump which might
//  be from a time period before the first packet of the file.
//  The least signficant 28 bits of the coarse time are obtained from TTE Time Words, while
//  the most significant 4 bits are obtained from the packet header.
//  This program might calculate incorrect times near the time of the least significant
//  28 bits roll over -- this happens every 310 days.

//  If a TTE packet contains Data Words before a Time Word, this program handles those
//  Data Words.
//  If a TTE packet lacks a Time Word, but an earlier TTE packet from the same "chunk"
//  contained a Time Word, this program calculates the time of the Data Words of the
//  packet that lacks a Time Word.
//  The case that is NOT handled is TTE packets that precede the first TTE packet of
//  a "chunk" which contains a TTE Time Word.


//  TTE formats -- deduced from my code -- should be compared to the documentation:

//  format of TTE Coarse Time word:

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


//   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***

//  All arguments are INPUT, output is by writing to standard out and the two files.

void ProcessTTE (

   uint32_t PacketData [],
   uint16_t PacketDataLength,
   uint32_t HeaderCoarseTime,
   uint16_t HeaderFineTime,
   uint16_t SequenceCount,
   FILE * ptr_to_SummaryFile,
   FILE * ptr_to_TTE_File

) {

   //   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***


   //  MostRecentTTE_TimeWord records the most recent TTE Time Word, including across
   //  packets if needed.   The initial value is an illegal value (see below)
   //  used to mark that the value is invalid.
   //  These variables have to be "static" because this routine is called once per packet,
   //  but these variables need to retain information across packets, and thus across calls
   //  to this routine.

   static  uint32_t  MostRecentTTE_TimeWord = 0x0;

   static  uint32_t  OverallDataWordCount = 0;

   static long double  PREV_HeaderTime = -99.99;


   uint32_t  FullCoarseTime;

   long double  HeaderTime;

   uint16_t chan;
   uint16_t det;
   uint16_t FineTime;

   uint32_t i_word;

   uint32_t  Word4;
   uint8_t LeadingNibble;


   long double  JulianDay;
   int32_t  year, month, day, hours, minutes;
   long double  seconds;

   const long long unsigned int Ticks_per_Coarse = 50000;

   uint64_t UnifiedTime_Beg;
   uint64_t UnifiedTime_End;


   //  these variables apply to a single TTE packet
   //  (but don't need to be initialized;  except some to prevent compiler warning messages)

   uint32_t  LastDataWord = 0;
   uint32_t  LastTimeWord = 0;
   long double  FirstTime = 0.0;
   long double  LastTime = 0.0;

   long double DataTime;

   char TTE_TimeCase_Beg [3];
   char TTE_TimeCase_End [3];




   //  Initializations done for each packet:

   uint32_t SumTTE_Counts [14] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

   uint32_t   DataWordCount = 0;
   uint32_t  TimeWordCount = 0;

   _Bool  DataWordPreceedsFirstTimeWord = false;
   _Bool  DataWordFollowsLastTimeWord = false;

   uint32_t  PreceedingTimeWordCount = 0;
   uint32_t  FollowingTimeWordCount = 0;


   //  These are illegal values since the most significant nibble of a Time Word
   //  must be 0xF and the most significant nibble of a Data Word can never be 0xD or larger
   //  because the most significant 16 bits can never be larger than 50000 = 0xC350.

   uint32_t  FirstTimeWord = 0x0;
   uint32_t  FirstDataWord = 0xFFFFFFFF;



   //   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***


   //  a large gap ( > 1 s) between TTE packets means that we have
   //  entered a different "chunk" of TTE and so the last TTE Time word
   //  from the previous TTE packet doesn't apply to the current TTE packet.
   //  For example, for a trigger, the FSW pauses several seconds in between
   //  outputting the prompt TTE and dumping the pre-trigger FIFO buffer.

   HeaderTime = FloatTime_from_CoarseFine ( HeaderCoarseTime, HeaderFineTime );

   //  The separation time must be more than 1, otherwise will pick up the packet
   //  that is ejected by the FPGA stale time when data flow ends.
   //  With the FSW design for triggers and TTE Boxes, etc., 2.5 (guess 2.8)
   //  works to distinguish chunks of TTE data.

   if (  HeaderTime - PREV_HeaderTime  >  1.1 )  {

      MostRecentTTE_TimeWord = 0x0;

      JulianDay = GBM_MET_Time_to_JulianDay ( HeaderCoarseTime, HeaderFineTime );
      JulianDay_to_Calendar_subr ( JulianDay, &year, &month, &day, &hours, &minutes, &seconds );

      fprintf (ptr_to_SummaryFile, "\n\nNew chunk of TTE Data: The Header Time is: \n" );
      fprintf (ptr_to_SummaryFile, "Coarse: %10u, Fine: %7u, MET as float: %18.6Lf\n",
               HeaderCoarseTime, HeaderFineTime, HeaderTime );
      fprintf (ptr_to_SummaryFile, "which is  JD %21.11Lf  = %4d:%02d:%02d at %02d:%02d:%9Lf\n\n",
               JulianDay, year, month, day, hours, minutes, seconds );

      fprintf (ptr_to_TTE_File, "\n\nNew chunk of TTE Data: The Header Time is: \n" );
      fprintf (ptr_to_TTE_File, "Coarse: %10u, Fine: %7u, MET as float: %18.6Lf\n",
               HeaderCoarseTime, HeaderFineTime, HeaderTime );
      fprintf (ptr_to_TTE_File, "which is  JD %21.11Lf  = %4d:%02d:%02d at %02d:%02d:%9Lf\n\n",
               JulianDay, year, month, day, hours, minutes, seconds );

   }

   PREV_HeaderTime = HeaderTime;





   // For TTE, we go through the packet, identifying the first and last
   // time words, and the first and last data words.  These are used
   // to calculate the range of times of the data words in the packet.
   // TTE is natively big-endian, and so needs byte swapping...

   for ( i_word=0; i_word < PacketDataLength / 4; i_word++ ) {  //  loop over TTE words


      //  The TTE data natively consists of 4-byte big-endian words.
      //  Because the TTE data was read in 4-byte words on an Intel processor, which is
      //  little-endian, it needs to be byte-swapped.

      Word4 = ByteSwap_4 ( PacketData [i_word] );


      //  Identify whether this TTE Word is a TTE Time Word or a TTE Data Word by the value
      //  of its Leading (i.e., most-significant) Nibble.    The most-significant nibble of
      //  a TTE Time Word must be 0xF, while the most-significant nibble of a TTE Data Word,
      //  can never be >= 0xD because the most-significant 16 bits are the fine-time field and
      //  hence can never be larger than 50,000 = 0xC350.

      LeadingNibble = (unsigned char)  ( Word4 >> 28 );

      if ( LeadingNibble == 0xF ) {  //  Time or Data Word ?

         // this is a Time Word:

         TimeWordCount++;


         //  if we don't yet have a data word from this packet, increment the
         //  count of Time Words in front of the first Data Word:

         if ( DataWordCount  ==  0)  PreceedingTimeWordCount++;

         //  increment the count of Time Words after the last Data Word.
         //  if this is a mistake, it will be fixed when we encounter another
         //  Data Word in this packet:

         FollowingTimeWordCount++;

         if ( FirstTimeWord == 0x0 ) FirstTimeWord = Word4;

         LastTimeWord = Word4;
         DataWordFollowsLastTimeWord = false;
         MostRecentTTE_TimeWord = Word4;

      } else {  //  Time or Data Word ?

         // this is a Data Word:

         //  reset the count of TimeWords following the last Data Word because
         //  all time words so far encountered don't follow the last data word:

         FollowingTimeWordCount = 0;

         DataWordCount++;
         OverallDataWordCount++;

         if ( FirstDataWord == 0xFFFFFFFF )  FirstDataWord = Word4;
         LastDataWord = Word4;
         if ( FirstTimeWord == 0x0 )  DataWordPreceedsFirstTimeWord = true;
         DataWordFollowsLastTimeWord = true;


         //  Now analyze the data in the Data Word
         //  1) sum the counts for each detector.
         //  2) if we have a preceeding Time Word, use it to output the time of this Data Word to the .tte file
         //  bits 0 to 7 are the channel and bits 8 to 11 are the detector:

         chan = Word4 & 0x7F;
         det = (Word4 >> 8) & 0xF;

         FineTime = (short unsigned int)  ( Word4 >> 16 );


         if ( det > 14 ) {
             printf ( "\n\n ############  Bad Detector Value: %u\n\n", det );
            fprintf (ptr_to_SummaryFile, "\n\n ############  Bad Detector Value: %u\n\n", det );
         } else {

            SumTTE_Counts [det] ++;

         }     // valid det num ?


         if ( chan >= NUM_SPEC_CHAN ) {
             printf ( "\n\n ############  Bad Channel Value: %u\n\n", chan );
            fprintf (ptr_to_SummaryFile, "\n\n ############  Bad Channel Value: %u\n\n", chan );
         }     // valid channel ?

         if ( FineTime >= 50000 ) {
             printf ( "\n\n ############  Bad Fine Time Value: %u\n\n", FineTime );
            fprintf (ptr_to_SummaryFile, "\n\n ############  Bad Fine Time Value: %u\n\n", FineTime );
         }     // valid channel ?


         //  Output each TTE data to a separate line in the "TTE file":

         //  0x0 identifies the condition that we have not yet encountered a TTE Time Word.
         //  If we have a TTE Time Word, then output this TTE Data Word:

         if ( MostRecentTTE_TimeWord  !=  0x0 ) {  // have TTE time word ?

            //  The full coarse time is obtained by combining bits from two sources:
            /// transferring the four MSB from the header coarse time into the four MSB of of the coarse
            //  time and transferrring the least-significant 28 bits of the TTE Time Word as the 28 LSB
            //  of the coarse time.
            //  Also see Time_from_TTE_Data.

            FullCoarseTime = ( HeaderCoarseTime & 0xF0000000 )  |  ( MostRecentTTE_TimeWord & 0x0FFFFFFF );

            DataTime = Time_from_TTE_Data ( HeaderCoarseTime, MostRecentTTE_TimeWord, Word4 );

            fprintf ( ptr_to_TTE_File, "%7u %10u  0x%08X  %10u  %5u  %18.6Lf  %2u  %3u\n",
               SequenceCount, OverallDataWordCount,
               (MostRecentTTE_TimeWord  & 0x0FFFFFFF), FullCoarseTime, (Word4 >> 16),
               DataTime, det, chan );

         } else {  // have TTE time word ?

            //  We don't have a TTE time word yet, but output what we can:

            fprintf ( ptr_to_TTE_File, "%7u %10u coarse time not avail  %5u  time not available  %2u  %3u\n",
               SequenceCount, OverallDataWordCount,
               (Word4 >> 16), det, chan );

         }  // have TTE time word ?


      }  //  Time or Data Word ?

   }  //  loop over TTE words



   //  Now report the contents of this packet, e.g., the time range of the data in
   //  this packet.


   fprintf ( ptr_to_SummaryFile,  "%5u Time Words and %5u Data Words\n", TimeWordCount, DataWordCount );
   if ( DataWordCount  !=  0 )
      fprintf ( ptr_to_SummaryFile,  "First and Last Data Words:  0x%08X  0x%08X\n", FirstDataWord, LastDataWord );
   if ( TimeWordCount  !=  0 )
      fprintf ( ptr_to_SummaryFile,  "First and Last Time Words:  0x%08X  0x%08X\n", FirstTimeWord, LastTimeWord );



   //  First try to figure the time of the First Data Word in the packet:

   UnifiedTime_Beg = 0;
   strncpy ( TTE_TimeCase_Beg, "u ", sizeof (TTE_TimeCase_Beg) );

   if ( DataWordCount !=0 ) {
      if ( TimeWordCount != 0 ) {

         //  Have both data and time words in this packet so can decode times of the data words:

         FirstTime = Time_from_TTE_Data  ( HeaderCoarseTime, FirstTimeWord, FirstDataWord );
         UnifiedTime_Beg = GBM_UnifiedTime_from_TTE_Data ( HeaderCoarseTime, FirstTimeWord, FirstDataWord );

         if ( DataWordPreceedsFirstTimeWord ) {

            //  The first data word in this packet is before the first time word in this packet:
            //  We have to subtract 0.1 s because we are using the Time Word that
            //  follows the Data Word and thus reports the next coarse time.

            FirstTime -= 0.1;
            UnifiedTime_Beg -= Ticks_per_Coarse;
            strncpy ( TTE_TimeCase_Beg, "a ", sizeof (TTE_TimeCase_Beg) );

         } else {

            strncpy ( TTE_TimeCase_Beg, "b ", sizeof (TTE_TimeCase_Beg) );

            if ( PreceedingTimeWordCount  >  1 ) {

               //  we used the first time word, but actually this isn't the time word
               //  immediately before the first data word, so the correction is
               //  mulitples of the coarse time unit (i.e., 0.1 s).

               strncpy ( TTE_TimeCase_Beg, "b2", sizeof (TTE_TimeCase_Beg) );
               FirstTime += 0.1 * (PreceedingTimeWordCount - 1);
               UnifiedTime_Beg += Ticks_per_Coarse * (PreceedingTimeWordCount - 1);

            }
         }

      } else {

         //  lacking a time word in this packet, see if we have a Time Word from an earlier TTE packet:

         if ( MostRecentTTE_TimeWord != 0 ) {

            strncpy ( TTE_TimeCase_Beg, "c ", sizeof (TTE_TimeCase_Beg) );
            FirstTime = Time_from_TTE_Data  ( HeaderCoarseTime, MostRecentTTE_TimeWord, FirstDataWord );
            UnifiedTime_Beg = GBM_UnifiedTime_from_TTE_Data ( HeaderCoarseTime, MostRecentTTE_TimeWord, FirstDataWord );

         }
      }
   }

   if ( TTE_TimeCase_Beg [0]  !=  'u' ) {
      fprintf (ptr_to_SummaryFile, "Times of data words: (Case %2s) MET %18.6Lf", TTE_TimeCase_Beg, FirstTime);
   } else {
      fprintf (ptr_to_SummaryFile, "Times of data words: (Case u )  [time not decoded]   ");
   }



   //  Next try to figure the time of the Last Data Word in the packet:

   UnifiedTime_End = 0;
   strncpy ( TTE_TimeCase_End, "U ", sizeof (TTE_TimeCase_End) );

   if ( DataWordCount !=0 ) {

      if ( TimeWordCount != 0 ) {

         //  Have both data and time words in this packet so can decode times of the data words:

         LastTime = Time_from_TTE_Data ( HeaderCoarseTime, LastTimeWord, LastDataWord );
         UnifiedTime_End = GBM_UnifiedTime_from_TTE_Data ( HeaderCoarseTime, LastTimeWord, LastDataWord );

         if ( DataWordFollowsLastTimeWord ) {

            strncpy ( TTE_TimeCase_End, "A ", sizeof (TTE_TimeCase_End) );

         } else {

            strncpy ( TTE_TimeCase_End, "B ", sizeof (TTE_TimeCase_End) );

            //  We have to subtract 0.1 s because we are using a Time Word that
            //  follows the Data Word and thus reports a later coarse time.
            //  We have to subtract 0.1 s for each Time Word that follows the last Data Word
            //  since we use the time of the very last Time Word, rather than the Time Word
            //  immediately following the last Data Word.

            LastTime -= 0.1 * FollowingTimeWordCount;
            UnifiedTime_End -= Ticks_per_Coarse;
            if ( FollowingTimeWordCount  >  1 )  strncpy ( TTE_TimeCase_End, "B2", sizeof (TTE_TimeCase_End) );

         }

      } else {

          //  lacking a time word in this packet, see if we have a Time Word from an earlier TTE packet:

         if ( MostRecentTTE_TimeWord != 0 ) {

            strncpy ( TTE_TimeCase_End, "C ", sizeof (TTE_TimeCase_End) );
            LastTime = Time_from_TTE_Data ( HeaderCoarseTime, MostRecentTTE_TimeWord, LastDataWord );
            UnifiedTime_End = GBM_UnifiedTime_from_TTE_Data ( HeaderCoarseTime, MostRecentTTE_TimeWord, LastDataWord );

         }
      }
   }

   if ( TTE_TimeCase_End [0]  !=  'U' ) {
      fprintf (ptr_to_SummaryFile, " to (Case %2s) MET %18.6Lf\n", TTE_TimeCase_End, LastTime);
   } else {
      fprintf (ptr_to_SummaryFile, "    (Case U )   [time not decoded]\n" );
   }


  //  Now output the times in standard units:

   if ( TTE_TimeCase_Beg [0]  !=  'u' ) {
      JulianDay = GBM_UnifiedTime_to_JulianDay ( UnifiedTime_Beg );
      JulianDay_to_Calendar_subr ( JulianDay, &year, &month, &day, &hours, &minutes, &seconds );
      fprintf (ptr_to_SummaryFile, " ... which is FROM JD %21.11Lf  = %4d:%02d:%02d at %02d:%02d:%9Lf\n",
               JulianDay, year, month, day, hours, minutes, seconds );
   }
   if ( TTE_TimeCase_End [0]  !=  'U' ) {
      JulianDay = GBM_UnifiedTime_to_JulianDay ( UnifiedTime_End );
      JulianDay_to_Calendar_subr ( JulianDay, &year, &month, &day, &hours, &minutes, &seconds );
      fprintf (ptr_to_SummaryFile, " .............. TO JD %21.11Lf  = %4d:%02d:%02d at %02d:%02d:%9Lf\n",
               JulianDay, year, month, day, hours, minutes, seconds );
   }



   fprintf (ptr_to_SummaryFile, "TTE summed from SPEC chan 0 to 127 for each Detector:\n Det 0 to  6:" );
   for (det=0; det <=  6; det++)  fprintf (ptr_to_SummaryFile, " %8u", SumTTE_Counts [det] );
   fprintf (ptr_to_SummaryFile, "\n Det 7 to 13:");
   for (det=7; det <= 13; det++)  fprintf (ptr_to_SummaryFile, " %8u", SumTTE_Counts [det] );
   fprintf (ptr_to_SummaryFile, "\n");


}  //  ProcessTTE ()
