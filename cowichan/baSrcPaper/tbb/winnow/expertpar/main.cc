/* winnow: weighted point selection
 *
 * input:
 *   matrix: an integer matrix, whose values are used as masses
 *   mask: a boolean matrix, showing which points are eligible for
 *     consideration
 *   nrows, ncols: number of rows and columns
 *   nelts: the number of points to select
 *
 * output:
 *   points: a vector of (x, y) points
 */
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <iostream>
#include <vector>
#include <time.h>
#include <stdio.h>

#include "tbb/tbb.h"

using namespace std;
using namespace tbb;

int is_bench = 0;
int n_threads = task_scheduler_init::default_num_threads();


static int* matrix;
static int* mask;
static pair<int, int> *points;
static pair<int, pair<int, int> > *values;
static int* count_per_line;
static int* total_count;

typedef blocked_range<size_t> range;

class ScanSum {
  int sum;

public:
  ScanSum(): sum(0) {}
  template<typename Tag>
  void operator()(range r, Tag) {
    int res = sum;
    for (size_t i = r.begin(); i != r.end(); ++i) {
      res += count_per_line[i];
      if (Tag::is_final_scan()) {
        total_count[i] = res;
      }
    }
    sum = res;
  }
  ScanSum(ScanSum& other, tbb::split) : sum(0) {}
  void reverse_join(ScanSum& other) { sum += other.sum; }
  void assign(ScanSum& other) { sum = other.sum; }
};

void winnow(int nrows, int ncols, int nelts) {
  int count = 0;

  count = parallel_reduce(
    range(0, nrows), 0,
    [=](range r, int result)->int {
      for (size_t i = r.begin(); i != r.end(); i++) {
        int cur = 0;
        for (int j = 0; j < ncols; j++) {
          // if (is_bench) {
            // mask[i*ncols + j] = ((i * j) % (ncols + 1)) == 1;
          // }
          cur += mask[i*ncols + j];
        }
        result += count_per_line[i + 1] = cur;
      }
      return result;
    },
    [](int x, int y)->int {
      return x + y;
    });

  ScanSum scan_sum;
  tbb::parallel_scan(
      range(0, nrows + 1),
      scan_sum);

  tbb::parallel_for(
      range(0, nrows),
      [=](range r) {
        for (size_t i = r.begin(); i != r.end(); i++) {
          int count = total_count[i];
          for (int j = 0; j < ncols; j++) {
            if (mask[i*ncols + j]) {
              values[count] = (make_pair(matrix[i*ncols + j],
                  make_pair(i, j)));
              count++;
            }
          }
        }
      });


  tbb::parallel_sort (values, values + count);

  size_t n = count;
  size_t chunk = n / nelts;

  // printf("winEL:%i\n",n);
  
  for (int i = 0; i < nelts; i++) {
    int index = i * chunk;
    points[i] = values[index].second;
  }
}

void read_matrix(int nrows, int ncols) {
  for (int i = 0; i < nrows; i++) {
    for (int j = 0; j < ncols; j++) {
      int v;
      cin >> v;
      matrix[i*ncols + j] = v;
    }
  }
}

void read_mask(int nrows, int ncols) {
  for (int i = 0; i < nrows; i++) {
    for (int j = 0; j < ncols; j++) {
      int v;
      cin >> v;
      mask[i*ncols + j] = v;
    }
  }
}

inline void FillOnTheFly( int nrows, int ncols, int thresh ){
  int threshInverse = 100 / thresh;
  
  tbb::parallel_for( int(0), (nrows*ncols), [&](int ix){
    if( (ix % threshInverse) == 0 )
    {
      mask[ix] = 1;
    }else{
      mask[ix] = 0;
    }
    
    matrix[ix] = ix % 100;
  } );
}

int main(int argc, char** argv) {
  int nrows, ncols, nelts, thresh;
  struct timespec start, stop;
  double accum;

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--is_bench")) {
      is_bench = 1;
    } else if (!strcmp(argv[i], "--threads")) {
      sscanf(argv[i + 1], "%d", &n_threads);
      i++;
    }
  }

  task_scheduler_init init(n_threads);

  scanf("%d%d", &nrows, &ncols);
  matrix = (int *) malloc (sizeof (int) * nrows * ncols);
  mask = (int *) malloc (sizeof (int) * nrows * ncols);
  
  total_count = (int *) malloc (sizeof (int) * (nrows + 1));
  memset (total_count, 0, sizeof (int) * (nrows + 1));

  count_per_line = (int *) malloc (sizeof (int) * (nrows + 1));
  memset (count_per_line, 0, sizeof (int) * (nrows + 1));

  if (!is_bench) {
    read_matrix(nrows, ncols);
    read_mask(nrows, ncols);
  }

  scanf("%d", &nelts);
  points = (pair <int, int> *) malloc (sizeof (pair <int, int>) * nelts);
  values = (pair <int, pair <int, int> > *) malloc (sizeof (pair <int, pair <int, int> >) * nrows * ncols);
  
  
  if(is_bench)
  {
    scanf("%d", &thresh);
    FillOnTheFly( nrows, ncols, thresh );
  }
  
  if( clock_gettime( CLOCK_MONOTONIC_RAW, &start) == -1 ) {
    perror( "clock gettime error 1" );
    exit( EXIT_FAILURE );
  }

  winnow(nrows, ncols, nelts);
  
  if( clock_gettime( CLOCK_MONOTONIC_RAW, &stop) == -1 ) {
    perror( "clock gettime error 2" );
    exit( EXIT_FAILURE );
  }
  
  accum = ( stop.tv_sec - start.tv_sec ) + ( stop.tv_nsec - start.tv_nsec ) / 1e9;
  
  

  FILE* fp = fopen("./measurements.txt", "a");
  
  if( !fp ) {
      perror("File opening for benchmark results failed");
      return EXIT_FAILURE;
  }
  // Lang, Problem, rows, cols, thresh, winnow_nelts, jobs, time
  fprintf( fp, "TBB   ,Winnow ,%5i,%5i,%3i,%5i,%2i,%.9lf, isBench:%d\n", nrows, ncols, thresh, nelts, n_threads, accum, is_bench );
  fclose ( fp );
  

  if (!is_bench) {
    printf("%d\n", nelts);

    for (int i = 0; i < nelts; i++) {
      printf("%d %d\n", points[i].first, points[i].second);
    }
    printf("\n");
  }

  return 0;
}
