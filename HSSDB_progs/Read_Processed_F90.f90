program Read_Processed


   use, intrinsic :: ISO_FORTRAN_ENV

   use, intrinsic :: ISO_C_BINDING

   implicit none


   ! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

!   typedef struct   Processed_TTE_type {
!      uint32_t CoarseTime;
!      uint16_t FineTime;
!      uint16_t  Detector;
!      uint16_t  SpecChannel;
!!      //uint64_t  Time_in_OneVariable;
!   uint16_t PADDING;   // required for compatibility with other languages
!   }  Processed_TTE_type;


   !  should be unsigned integers, but those are not available in Fortran.
   !  This will need "fixing" for both times.  The most-significant
   !  bit should not get set for the other items.

   type, bind (C) :: Processed_TTE_type

      integer (C_INT32_T) :: CoarseTime
      integer (C_INT16_T) :: FineTime
      integer (C_INT16_T) :: Detector
      integer (C_INT16_T) :: SpecChannel
      integer (C_INT16_T) :: PADDING

   end type

   type (Processed_TTE_type) :: TTE_data

   integer :: in_LUN
   integer :: i

   integer (int64) :: CoarseTime_64
   integer (int32) :: FineTime_32

   ! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


   open ( newunit=in_LUN, file="Processed_TTE.dat",  status="old", access="stream",   &
             form="unformatted", action="read" )


   do i=1,1000

      read (in_LUN)  TTE_data

      CoarseTime_64 = TTE_Data % CoarseTime
      if ( CoarseTime_64 < 0_int64 )  CoarseTime_64 = CoarseTime_64  +  2_int64 ** 32_int64

      FineTime_32 = TTE_Data % FineTime
      if ( FineTime_32 < 0_int32 )  FineTime_32 = FineTime_32  +  2_int32 ** 16_int32

      write (*, '( I10, 2X, I5, 3X, I2, 2X, I3 )' )   &
         CoarseTime_64, FineTime_32, TTE_Data % Detector, TTE_Data % SpecChannel

   end do


end program Read_Processed
