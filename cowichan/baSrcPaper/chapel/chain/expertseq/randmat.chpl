/* randmat: random number generation
 * 
 * input:
 *   nrows, ncols: the number of rows and columns
 *   s: the seed
 *
 * output:
 *   matrix: an nrows x ncols integer matrix
 */

module Randmat {

use Config;

proc randmat(nrows: int, ncols: int, s: uint(32)) {
  const LCG_A: uint(32) = 1664525;
  const LCG_C: uint(32) = 1013904223;
  const RowSpace = {1..nrows};
  const ColSpace = {1..ncols};
  for i in RowSpace {
    var seed: uint(32);
    seed = s + (i : uint(32)) - 1;
    for j in ColSpace {
      seed = LCG_A * seed + LCG_C;
      matrix[i, j] = abs(seed) % 100;
    }
  }
}

}
