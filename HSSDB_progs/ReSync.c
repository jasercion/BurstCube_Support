
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.


//  Michael S. Briggs, 2004 Oct 8, UAH / NSSTC.
//  Michael S. Briggs, 2003 Nov 18 & Nov 26.
//  MSB, 2007 Sept 18, improved declarations.

//  The purpose of this routine is to place the "read point" just in front
//  of the next GBM packet.   This is for files of GBM Science (HSSDB) data.

//  2008 July 4--6.
//  Now handles data from both I&T and Level 0 data re-formatted by the MOC.
//  In the case of I&T data, each GBM data packet by design is preceded with with
//  a 4-byte sync word, 0x352EF853.    In the case of Level 0 data from,
//  GBM packets are preceded by a 12-byte MOC packet header.   We can still use
//  this as a sync feature since the first 4 bytes of the MOC header always
//  have the value 0x53498900.   Of course, we also have to skip an additional
//  8 bytes.  This routine skips those bytes to position the "read point" just before
//  GBM data.  This way other routines don't have to know whether the data file is
//  I&T data or Level 0 data -- they can just read from the "read point"
//  established by this routine.

//  MSB, 2004 Aug 11: the 4-byte sync word is changed from 0x1ACFFC1D to
//  the new value of 0x352EF853, reflecting a change made approx 2004 May 17

//  MSB, 2004 Oct 1:
//  1) switch to read the file one byte at a time in order to handle the case
//  that there are an odd number of bytes in front of the sync word.
//  2) instead of just returning whether or not there were excess non-zero bytes,
//  we count the number of excess non-zero bytes in front of the sync word.

//  Used with FileScan.c.  Adapted from the previous version of FileScan.c
//  Starting from the current position in reading the file (which might be
//  before any bytes have been read), this subroutine reads up to and including
//  the next 4 byte sync word 0x352EF853.   As the return value, it reports
//  the number of bytes in front of the sync word.


//  returns TRUE if the sync word is found, FALSE otherwise

//  If an error causes a return of FALSE, the two output arguments won't be valid.

//  arguments:
   //    file_pointer:      input only:  pointer to file to read from,
   //    ExcessBytes:       output only: count of all excess bytes (i.e., in front of the sync word)
   //    ExcessNonZeroCnt:  output only: Count of excess bytes that are non-zero.   Of course, if there are
   //                                    no excess bytes in front of the sync word, this arg will be zero.



#include "HSSDB_Progs_Header.h"


static  _Bool  ReSync_FileReadBAD ( FILE * file_pointer,  size_t num_read );


typedef enum {

   DATAORIGIN_UNDEFINED,
   DATAORIGIN_I_AND_T,
   DATAORIGIN_LEVEL0

} Data_Origin_type;




_Bool  ReSync (

   FILE * file_pointer,
   uint32_t * ExcessBytes,
   uint32_t * ExcessNonZeroCnt

) {


   _Bool FoundSyncWord = false;

   size_t num_read;
   uint8_t Byte1, Byte2, Byte3, Byte4;
   static uint8_t CF_Byte1, CF_Byte2, CF_Byte3, CF_Byte4;
   uint32_t TwoWords [2];

   char DataOriginChars [3];
   static _Bool  FirstCall = true;
   static Data_Origin_type  DataOrigin = DATAORIGIN_UNDEFINED;


   uint32_t TotBytesRead = 0;


   // ********************************************************************


   //  On the first call, ask the user about the origin of the data file --
   //  from I&T, either from our SIIS or GD, or Level 0 data from the MOC
   //  pipeline, regardless of whether the SC was on the ground or in orbit.
   //  The two data origins have different file layouts and this routine
   //  will sync to the GBM HSSDB packets differently, as explained in the
   //  comments.

   if ( FirstCall ) {

      FirstCall = false;

      printf ( "\nIs the origin of the data file I&T or Level 0 from the MOC?\n" );
      printf ( "Input either IT or L0: " );
      scanf ( "%2s", DataOriginChars );
      printf ( "\n" );

      if ( DataOriginChars [0] == 'i' )  DataOriginChars [0] = 'I';
      if ( DataOriginChars [0] == 'l' )  DataOriginChars [0] = 'L';
      if ( DataOriginChars [1] == 't' )  DataOriginChars [1] = 'T';

      if ( strcmp ( DataOriginChars, "IT" ) == 0 ) {

         DataOrigin = DATAORIGIN_I_AND_T;
         CF_Byte1 = 0x35;
         CF_Byte2 = 0x2E;
         CF_Byte3 = 0xF8;
         CF_Byte4 = 0x53;

      }

      if ( strcmp ( DataOriginChars, "L0" ) == 0 ) {

         DataOrigin = DATAORIGIN_LEVEL0;
         CF_Byte1 = 0x53;
         CF_Byte2 = 0x49;
         CF_Byte3 = 0x89;
         CF_Byte4 = 0x00;

      }

      if ( DataOrigin == DATAORIGIN_UNDEFINED ) {
         printf ( "\n'%s' unrecognized!\n", DataOriginChars );
         exit (17);
      }

   }  // FirstCall ?


   if ( DataOrigin == DATAORIGIN_UNDEFINED )  exit (19);


   * ExcessBytes = 0;
   * ExcessNonZeroCnt = 0;


   // Scan through file skipping everything up to the next 4-byte sync word.
   // Junk at the start of a file is either 1A) non-data,
   // or 1B) a trailing fragment of a packet.   If in between packets, the junk
   // is 2) excess bytes of unknown origin.
   //
   // The sync word is 0x352EF853 -- since we are reading one byte at a time
   // there are no endian issues.



   while ( !FoundSyncWord ) {


      Byte2 = 0x00;   // see end of while loop for explanation
      Byte3 = 0x00;
      Byte4 = 0x00;


      num_read = fread ( &Byte1, 1, 1, file_pointer );

      if ( ReSync_FileReadBAD ( file_pointer, num_read ) )  return false;
      TotBytesRead ++;


      // test whether the value of the byte just read is consistent with the first byte of
      // the sync word -- if yes, read another byte and test it against the second byte of the
      // sync word -- if no, go to the end of the loop, which will repeat the action of reading
      // a byte and testing it against the first byte of the sync word.

      if ( Byte1 == CF_Byte1 ) {  // Byte1 consistent with sync word ?

         num_read = fread ( &Byte2, 1, 1, file_pointer );

         if ( ReSync_FileReadBAD ( file_pointer, num_read ) )  return false;
         TotBytesRead ++;


         // if this might be the second byte of the sync word, try for the 3rd byte:
         if ( Byte2 == CF_Byte2 ) {  // Byte2 consistent with sync word ?

            num_read = fread ( &Byte3, 1, 1, file_pointer );

            if ( ReSync_FileReadBAD ( file_pointer, num_read ) )  return false;
            TotBytesRead ++;


           // if this might be the third byte of the sync word, try for the 4th and last byte:
            if ( Byte3 == CF_Byte3 ) {  // Byte3 consistent with sync word ?

               num_read = fread ( &Byte4, 1, 1, file_pointer );

               if ( ReSync_FileReadBAD ( file_pointer, num_read ) )  return false;
               TotBytesRead ++;


               // if this 4th and last byte also matches the sync word, then we have found the sync word !

               if ( Byte4 == CF_Byte4 ) {  // Byte4 consistent with sync word ?

                  FoundSyncWord = true;

               }  // Byte4 consistent with sync word ?

            }  // Byte3 consistent with sync word ?

         }  // Byte2 consistent with sync word ?

      }  // Byte1 consistent with sync word ?


      //  If we get here, we read some number of bytes between 1 and 4.
      //  If we didn't find the sync word, test whether any of the bytes that we read are
      //  non-zero.
      //  Since all of the bytes that might not have been read were initialized to zero,
      //  we can simply test all of the bytes -- the only way that a byte can be non-zero
      //  is if a non-zero byte was read from the file.

      //  Note with new "sync" word for Level 0 data, byte 4 is 0x00, but this is OK since
      //  it is the last byte in the sync word -- if we get reach this value we will have
      //  the sync word and won't execute the contents of this IF statement.

      if ( !FoundSyncWord ) {
         if ( Byte1  !=  0x00 )  (* ExcessNonZeroCnt) ++;
         if ( Byte2  !=  0x00 )  (* ExcessNonZeroCnt) ++;
         if ( Byte3  !=  0x00 )  (* ExcessNonZeroCnt) ++;
         if ( Byte4  !=  0x00 )  (* ExcessNonZeroCnt) ++;
      }


   }  //  while !FoundSyncWord


   if ( TotBytesRead > 4U ) {
      *ExcessBytes = TotBytesRead - 4U;
   } else {
      *ExcessBytes = 0;
   }


   //  The "feature" that I am synching up with in the Level 0 data really isn't
   //  a 4-byte sync word, but rather a 8-byte header.
   //  So having found the first 4 bytes of the header, which have an
   //  invariant value, there are 8 more bytes to skip -- read as two 4-bytes words.

   if ( DataOrigin == DATAORIGIN_LEVEL0 ) {
      num_read = fread ( &TwoWords, 4, 2, file_pointer );
      if ( num_read != 2 ) exit ( 17 );
   }


//  printf ( "\n%u  %u  %u\n", FoundSyncWord, *ExcessBytes, *ExcessNonZeroCnt );

   return FoundSyncWord;


}  //  ReSync ()



// ***********************************************************************

static  _Bool  ReSync_FileReadBAD ( FILE * file_pointer,  size_t num_read ) {

   if ( feof (file_pointer) ) {
      printf ( "\n EOF while searching for next sync word 1!\n" );
      return true;
   }

   if ( ferror (file_pointer) ) {
      printf ( "\n File Error searching next sync word 1!\n" );
      return true;
   }

   if ( num_read != 1 ) {
      printf ( "\nWrong number of items searching for sync word 1!\n" );
      return true;
   }


   //  all error tests passed, return that last read of the file was NOT bad:
   return false;


}  //  ReSync_FileReadBAD ()
