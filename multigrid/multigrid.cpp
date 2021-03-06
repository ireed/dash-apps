#include <unistd.h>
#include <iostream>
#include <sstream>
#include <cstddef>
#include <iomanip>
#include <cassert>
#include <vector>
#include <cstdio>

#include <libdash.h>
#include <dash/experimental/HaloMatrixWrapper.h>

#define WITHPNGOUTPUT 1
#ifdef WITHPNGOUTPUT
#include <png++/png.hpp>

uint32_t filenumber= 0;
const std::string filename("image");
const std::string suffix("image");

#endif /* WITHPNGOUTPUT */

using std::cout;
using std::endl;
using std::setw;
using std::vector;

using TeamT              = dash::Team;
using TeamSpecT          = dash::TeamSpec<2>;
using MatrixT            = dash::NArray<double,2>;
using StencilT           = dash::experimental::Stencil<2>;
using StencilSpecT       = dash::experimental::StencilSpec<2,4>;
using HaloMatrixWrapperT = dash::experimental::HaloMatrixWrapper<MatrixT, StencilSpecT>;
using CycleSpecT         = dash::experimental::CycleSpec<2>;
using Cycle              = dash::experimental::Cycle;

const StencilSpecT stencil_spec({
    StencilT(-1, 0), StencilT( 1, 0),
    StencilT( 0,-1), StencilT( 0, 1)});


struct Level {

    MatrixT grid;
    HaloMatrixWrapperT halo;

    Level( size_t w, size_t h, TeamT& team, TeamSpecT& teamspec ) :
        grid( dash::SizeSpec<2>( h, w ),
              dash::DistributionSpec<2>( dash::BLOCKED, dash::BLOCKED ), team, teamspec ),
        halo( grid, stencil_spec, CycleSpecT(Cycle::FIXED, Cycle::FIXED )) {}
    Level() = delete;

    ~Level() {}
};

#ifdef WITHPNGOUTPUT
void topng( const MatrixT& grid, double minvalue= 0.0, double maxvalue= 100.0 ) {

    if ( 0 != dash::myid() ) return;

    uint32_t minsize= 512;
    uint32_t f= 1;
    size_t w= grid.extent(1);
    size_t h= grid.extent(0);


    if ( w < minsize || h < minsize ) {

        f= ( w <= h ) ? w : h;
        f= 1+minsize/f;
    }

    png::image< png::rgb_pixel > image( h*f, w*f );
    for ( png::uint_32 y = 0; y < h; y++ ) {
        for ( png::uint_32 x = 0; x < w; x++ ) {

            double temp= grid[y][x];
            double r= (temp-minvalue)*255.0/(maxvalue-minvalue);
            double g= 0;
            double b= 255.0 - r;

            r= ( r > 255.0 ) ? 255.0 : ( r < 0.0 ) ? 0.0 : r ;
            g= ( g > 255.0 ) ? 255.0 : ( g < 0.0 ) ? 0.0 : g ;
            b= ( b > 255.0 ) ? 255.0 : ( b < 0.0 ) ? 0.0 : b ;

            for ( png::uint_32 yy = 0; yy < f; yy++ ) {
                for ( png::uint_32 xx = 0; xx < f; xx++ ) {
                    image[y*f+yy][x*f+xx] = png::rgb_pixel( (uint8_t) r, (uint8_t) g, (uint8_t) b );
                }
            }
        }
    }
    std::stringstream ss;
    ss << filename << std::setfill('0') << setw(2) << filenumber++ << ".png";
    image.write( ss.str().c_str() );
}
#endif /* WITHPNGOUTPUT */

void writeToPNG(const MatrixT& grid) {
#ifdef WITHPNGOUTPUT
  grid.barrier();

  if(0 == dash::myid())
    topng(grid, 0.0, 10.0);

  grid.barrier();
#endif /* WITHPNGOUTPUT */
}

void sanitycheck( const MatrixT& grid  ) {

    /* check if the sum of the local extents of the matrix blocks sum up to
     * the global extents, abort otherwise */
    dash::Array<size_t> sums( dash::Team::All().size(), dash::BLOCKED );
    sums.local[0]= grid.local.extent(0) * grid.local.extent(1);
    sums.barrier();

    if ( 0 != dash::myid() ) {

        dash::transform<size_t>(
            sums.lbegin(), sums.lend(), // first source
            sums.begin(), // second source
            sums.begin(), // destination
            dash::plus<size_t>() );
    }
    sums.barrier();

    if ( ( (size_t) sums[0] ) != grid.extent(0) * grid.extent(1) ) {

        if ( 0 == dash::myid() ) {
            cout << "ERROR: size mismatch: global size is " <<
                grid.extent(0) * grid.extent(1) << " == " << grid.extent(0) << "x" << grid.extent(1) <<
                " but sum of local sizes is " << (size_t) sums[0] << std::flush << endl;
        }

        dash::finalize();
        exit(0);
    }
}


void initgrid( MatrixT& grid ) {

    if ( 0 != dash::myid() ) return;

    /* do initialization only from unit 0 with global accesses.
    This is not the fastest way and a parallel routine would not be too
    complicated but it should demonstrate the possibility to trade
    convenience against performance. */

    size_t w= grid.extent(1);
    size_t h= grid.extent(0);

    cout << "assign values to finest level" << endl;

    for( size_t i = 0; i < h; ++i )
      for( size_t k = 0; k < w; ++k )
        grid[i][k] = 7.0;

    for( size_t i = h/5; i < h*3/4; ++i )
      for( size_t k = w*1/5; k < w*1/3; ++k )
        grid[i][k] = 3.0;
}


void setboundary( Level& level ) {

    /* do initialization only from unit 0 with global accesses.
    This is not the fastest way and a parallel routine would not be too
    complicated but it should demonstrate the possibility to trade
    convenience against performance. */


    level.halo.setFixedHalos([&level](const std::array<dash::default_index_t,2>& coords) {
      size_t w= level.grid.extent(1);
      size_t h= level.grid.extent(0);

      if(coords[0] == -1 && coords[1] < w/3 ||
         coords[0] < h*3/5 && coords[1] == -1) {
        return 10.0;
      }

      if(coords[0] >= h*1/5 && coords[0] < h*2/5 && coords[1] == w  ||
         coords[0] >= h*3/5 && coords[0] < h*4/5 && coords[1] == w ) {
        return 0.0;
      }


      return 5.0;
    });
}

void scaledown( Level& fine, Level& coarse ) {

    assert( (coarse.grid.extent(1))*2 == fine.grid.extent(1) );
    assert( (coarse.grid.extent(0))*2 == fine.grid.extent(0) );

    fine.halo.updateHalosAsync();

    const auto& extentc = coarse.grid.pattern().local_extents();
    const auto cornerc = coarse.grid.pattern().global( {0,0} );

    size_t startx= ( 0 == cornerc[1] % 2 ) ? 0 : 1;
    size_t starty= ( 0 == cornerc[0] % 2 ) ? 0 : 1;

    for ( size_t y= 0; y < extentc[0]-starty ; y++ ) {
        for ( size_t x= 0; x < extentc[1]-startx ; x++ ) {

            coarse.grid.local(starty+y, startx+x)= 0.25 * (
                fine.grid.local(starty+2*y  , startx+2*x  ) +
                fine.grid.local(starty+2*y  , startx+2*x+1) +
                fine.grid.local(starty+2*y+1, startx+2*x  ) +
                fine.grid.local(starty+2*y+1, startx+2*x+1) );
        }
    }

    fine.halo.waitHalosAsync();


    if ( 0 == cornerc[0] ) {

        // cout << "first row globally is boundary condition, nothing to do here" << endl;

    } else if ( !cornerc[0] ) {

        cout << dash::myid() << ": first row locally is even, thus need to do scale down with halo row" << endl;

    } // else nothing to do here


    if ( coarse.grid.extent(0) == cornerc[0] + extentc[0] ) {

        // cout << "last row globally is boundary condition, nothing to do here" << endl;

    } else if ( 1 == ( cornerc[0] + extentc[0] ) % 2 ) {

        cout << dash::myid() << ": last row locally is odd (following first row in next block is even), thus need to do scale down with halo row" << endl;
    } // else nothing to do

    if ( 0 == cornerc[1] ) {

        // cout << "first column globally is boundary condition, nothing to do here" << endl;

    } else if ( !cornerc[1] ) {

        cout << dash::myid() << ": first column locally is even, thus need to do scale down with halo column" << endl;

    } // else nothing to do here


    if ( coarse.grid.extent(1) == cornerc[1] + extentc[1] ) {

        // cout << "last column globally is boundary condition, nothing to do here" << endl;

    } else if ( 1 == ( cornerc[1] + extentc[1] ) % 2 ) {

        cout << dash::myid() << ": last column locally is odd (following first column in next block is even), thus need to do scale down with halo column" << endl;
    } // else nothing to do



    /* now do border elements if necessary */

    /* change to inner/outer scheme
         - nonblocking halo exchange if needed
         - check local indices to find start at even positions per dimension
         - check if first row/column needs to use halo
         - check if last row/column needs to use halo

    */


    dash::barrier();
}


void scaleup( Level& coarse, Level& fine ) {

    assert( (coarse.grid.extent(1))*2  == fine.grid.extent(1) );
    assert( (coarse.grid.extent(0))*2  == fine.grid.extent(0) );

    size_t w= coarse.grid.local.extent(1);
    size_t h= coarse.grid.local.extent(0);
    const auto cornerc= coarse.grid.pattern().global( {0,0} );
    size_t startx= ( 0 == cornerc[1] % 2) ? 0 : 1;
    size_t starty= ( 0 == cornerc[0] % 2) ? 0 : 1;

    for ( size_t y= 0; y < h ; y++ ) {
        for ( size_t x= 0; x < w ; x++ ) {

            double t= coarse.grid.local(starty+y,startx+x);
            fine.grid.local(starty+2*y  , startx+2*x  )= t;
            fine.grid.local(starty+2*y  , startx+2*x+1)= t;
            fine.grid.local(starty+2*y+1, startx+2*x  )= t;
            fine.grid.local(starty+2*y+1, startx+2*x+1)= t;
        }
    }

    dash::barrier();
}


double smoothen( Level& level ) {

    double res = 0.0;

    /// relaxation coeff.
    const double c = 1.0;

    /// begin pointer of local block, needed because halo border iterator is read-only
    auto gridlocalbegin = level.grid.lbegin();

    // async halo update
    level.halo.updateHalosAsync();

    // update inner

    size_t lw= level.grid.local.extent(1);
    size_t lh= level.grid.local.extent(0);

    /* the start value for both, the y loop and the x loop is 1 because either there is
    a border area next to the halo -- then the first column or row is covered below in
    the border update -- or there is an outside border -- then the first column or row
    contains the boundary values. */

    for ( size_t y = 1; y < lh - 1; y++ ) {

        double* p_here= &gridlocalbegin[ y*lw + 1 ];
        double* p_east= &gridlocalbegin[ y*lw+1 + 1 ];
        double* p_west= &gridlocalbegin[ y*lw-1 + 1 ];
        double* p_north= &gridlocalbegin[ (y-1)*lw + 1 ];
        double* p_south= &gridlocalbegin[ (y+1)*lw + 1];

        for ( size_t x = 1; x < lw - 1; x++ ) {

            double dtheta = ( *p_east + *p_west + *p_north + *p_south ) / 4.0 - *p_here ;
            *p_here += c * dtheta;
            res = std::max( res, std::fabs( dtheta ) );
            p_here++;
            p_east++;
            p_west++;
            p_north++;
            p_south++;
        }
    }

    // wait for async halo update
    level.halo.waitHalosAsync();

    auto bend = level.halo.bend();
    // update border area
    for( auto it = level.halo.bbegin(); it != bend; ++it ) {
        double dtheta = ( it.valueAt(2) + it.valueAt(3) +
                          it.valueAt(0) + it.valueAt(1) ) / 4.0 - *it;
        gridlocalbegin[ it.lpos() ] += c * dtheta;
        res = std::max( res, std::fabs( dtheta ) );
    }

    dash::Array<double> residuals( dash::Team::All().size(), dash::BLOCKED );
    residuals.local[0] = res;

    residuals.barrier();

    if ( 0 != dash::myid() ) {

        dash::transform<double>(
            residuals.lbegin(), residuals.lend(), // first source
            residuals.begin(), // second source
            residuals.begin(), // destination
            dash::max<double>() );
    }

    residuals.barrier();

    /* only unit 0 returns global residual */
    return residuals.local[0];
}



void v_cycle( vector<Level*>& levels, uint32_t inneriterations= 1 ) {

    writeToPNG( levels[0]->grid );

    for ( auto i= 1; i < levels.size(); i++ ) {

        double res = smoothen( *levels[i-1] );

        writeToPNG( levels[i-1]->grid );

        scaledown( *levels[i-1], *levels[i] );

        if ( 0 == dash::myid() ) {
            cout << "smoothen with residual " << res <<
                ", then scale down " << levels[i-1]->grid.extent(1) << "x" << levels[i-1]->grid.extent(1) <<
                " --> " << levels[i]->grid.extent(1)   << "x" << levels[i]->grid.extent(1) << endl;
        }

        writeToPNG( levels[i]->grid );
    }

    /* do a fixed number of smoothing steps until the residual on the coarsest grid is
    what you want on the finest level in the end. */
    for ( auto i= 0; i < inneriterations; i++ ) {

        double res= smoothen( *levels.back() );

        if ( 0 == dash::myid() ) {
            cout << "smoothen coarsest with residual " << res << endl;
        }
        writeToPNG( levels.back()->grid );
    }

    for ( auto i= levels.size()-1; i > 0; i-- ) {

        scaleup( *levels[i], *levels[i-1] );

        writeToPNG( levels[i-1]->grid );

        double res= smoothen( *levels[i-1] );

        if ( 0 == dash::myid() ) {
            cout << "scale up " << levels[i]->grid.extent(1)   << "x" << levels[i]->grid.extent(1) <<
                " --> " << levels[i-1]->grid.extent(1) << "x" << levels[i-1]->grid.extent(1) <<
                ", then smoothen with residual " << res << endl;
        }

        writeToPNG( levels[i-1]->grid );
    }
}


int main( int argc, char* argv[] ) {

    dash::init(&argc, &argv);

    TeamSpecT teamspec( TeamT::All().size(), 1 );
    teamspec.balance_extents();

    constexpr uint32_t howmanylevels= 10;
    vector<Level*> levels;
    levels.reserve( howmanylevels );

    /* create all grid levels, starting with the finest and ending with 4x4 */
    for ( auto l= 0; l < howmanylevels-3; l++ ) {

        if ( 0 == dash::myid() ) {
            cout << "level " << l << " is " << (1<<(howmanylevels-l)) << "x" << (1<<(howmanylevels-l)) << endl;
        }

        levels.push_back( new Level( (1<<(howmanylevels-l)), (1<<(howmanylevels-l)),
            dash::Team::All(), teamspec ) );

        dash::barrier();
        cout << "unit " << dash::myid() << " : " <<
            levels.back()->grid.local.extent(1) << " x " <<
            levels.back()->grid.local.extent(0) << endl;
        dash::barrier();


        if ( 0 == l )
            sanitycheck( levels[0]->grid );

        setboundary( *levels[l] );
    }

    dash::barrier();

    /* Fill finest level. Strictly, we don't need to set any initial values here
    but we do it for demonstration in the PNG images */
    initgrid( levels[0]->grid );

    dash::barrier();
    v_cycle( levels, 10 );

    double res= smoothen( *levels.front() );
    if ( 0 == dash::myid() ) {
        cout << "final residual " << res << endl;
    }

    writeToPNG( levels.front()->grid );

    dash::finalize();
}
