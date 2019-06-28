
#include "HSSDB_Progs_Header.h"


//   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***


int main ( int argc, char * argv [] ) {


   size_t  FileName_Length;
   char * Input_FileName_ptr;

   FILE * ptr_to_DetectorFile;

   TTE_Data_type  TTE_Data;

   uint32_t  i;
   size_t  num_read;

   // ********************************************************************************************

   if ( argc != 2 ) {
      printf ( "Bad number of command line arguments: %d\n", argc - 1 );
      return 1;
   }

   FileName_Length = strlen ( argv [1] );
   Input_FileName_ptr   = malloc ( FileName_Length );
   memcpy ( Input_FileName_ptr,  argv [1], FileName_Length );

   ptr_to_DetectorFile = fopen ( Input_FileName_ptr, "rb" );

   if (ptr_to_DetectorFile == NULL ) {
      printf ( "\n\nFailed to input TTE File.  Exiting!\n" );
      exit (2);
   }


   for ( i=0; i < 1000; i++ ) {

      num_read = fread ( &TTE_Data,
                         sizeof ( TTE_Data ),
                         1,
                         ptr_to_DetectorFile );

      if ( num_read == 1 ) {


         printf ( "%u  %u  %u\n", TTE_Data.CoarseTime, TTE_Data.FineTime, TTE_Data.SpecChannel );
      } else {

         printf ( "\n\nRead failed.  Exiting!\n" );
         exit (3);

      }

   }  // i


   return (0);


}  // main ()
