#include <libdash.h>
#include <iostream>
#include <cstdio>

using std::cout;
using std::endl;
using std::cin;
using uint     = unsigned int ;
using uchar    = unsigned char;
using MATRIX_T = uchar        ;

struct     InputPar { uint nrows, ncols; } in;
uint       percent;
static int myid;

#include "thresh.h"

/*
 * This function prints the content of a 2D matrix to std::out.
 * Datatypes are casted to <const uint> for readable output
 * (otherwise uchars would be printed as chars and not as numerics)
 */
inline void Print2D( NArray< bool, 2 > const & mat )
{
  if(0==myid){

    //cout << in.nrows << " " << in.ncols << "\n";

    for( int i = 0; i < mat.extent(0); i++ ) {
      for( int j = 0; j < mat.extent(1); j++ ) {
        cout << static_cast<const uint>( mat(i,j) )<< " ";
      }
      cout << "\n";
    }
  }
}

// unit0 reads the matrix with random values from std::in
template<typename T = MATRIX_T>
inline void ReadRandMat( NArray< T, 2 > & rand_mat )
{
  if(0 == myid){
    T tmp;
    for ( auto i : rand_mat ){
      scanf( "%u", &tmp );
      i = tmp;
    }
  }

  // wait for initialization of the matrix before continuing
  barrier( );
}

/*
 * One unit has the job to read in the parameters.
 * Because there's always a unit0, it reads the input parameter and
 * distributes them to the rest of the units.
 */
inline void ReadRowsNCols( )
{
  Shared<InputPar> input_transfer;

  if(0 == myid)
  {
    cin >> in.nrows;
    cin >> in.ncols;

    input_transfer.set(in);
  }
  input_transfer.barrier();
  in = input_transfer.get();
}

inline void ReadPercentage( )
{
  Shared<uint> percent_transfer;

  if(0 == myid)
  {
    cin >> percent;

    percent_transfer.set(percent);
  }
  percent_transfer.barrier();
  percent = percent_transfer.get();
}


int main( int argc, char* argv[] )
{
  dash::init( &argc, &argv );

  myid = dash::myid( );
  ReadRowsNCols( );

  NArray< MATRIX_T, 2 > rand_mat    ( in.nrows, in.ncols );
  NArray< bool    , 2 > thresh_mask ( in.nrows, in.ncols );

  ReadRandMat(rand_mat);
  ReadPercentage();

  Thresh( rand_mat, thresh_mask, in.nrows, in.ncols, percent );
  Print2D( thresh_mask );

  dash::finalize( );
}
