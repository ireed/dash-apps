/*
 * product: matrix-vector product
 *
 * input:
 *   nelts: the number of elements
 *   matrix: the real matrix
 *   vector: the real vector
 *
 * output:
 *   result: a real vector, whose values are the result of the product
 */
package main

import (
	"flag"
	"fmt"
	"runtime"
  "bufio"
  "os"
)

// #include <time.h>
// #include <stdio.h>
import "C"

var is_bench = flag.Bool("is_bench", false, "")

var matrix []float64
var vector []float64

func product(m, vec []float64, nelts int) (result []float64) {
	result = make([]float64, nelts)

	NP := runtime.GOMAXPROCS(0)
	work := make(chan int)
	done := make(chan bool)

	go func() {
		for i := 0; i < nelts; i++ {
			work <- i
		}
		close(work)
	}()

	for i := 0; i < NP; i++ {
		go func() {
			for i := range work {
				sum := 0.0
				for j := 0; j < nelts; j++ {
					sum += m[i*nelts+j] * vec[j]
				}
				result[i] = sum
			}
			done <- true
		}()
	}

	for i := 0; i < NP; i++ {
		<-done
	}
	return
}

func read_integer() int {
	var value int
	for true {
		var read, _ = fmt.Scanf("%d", &value)
		if read == 1 {
			break
		}
	}
	return value
}

func read_float64() float64 {
	var value float64
	for true {
		var read, _ = fmt.Scanf("%g", &value)
		if read == 1 {
			break
		}
	}
	return value
}

func read_matrix(nelts int) {
	for i := 0; i < nelts*nelts; i++ {
		matrix[i] = read_float64()
	}
}

func read_vector(nelts int) {
	for i := 0; i < nelts; i++ {
		vector[i] = read_float64()
	}
}

func main() {
	var nelts int

	flag.Parse()

	nelts = read_integer()
  matrix = make ([]float64, nelts*nelts)
  vector = make ([]float64, nelts)

	if !*is_bench {
		read_matrix(nelts)
		read_vector(nelts)
	}

  var start, stop C.struct_timespec
  var accum C.double
  
  if( C.clock_gettime( C.CLOCK_MONOTONIC_RAW, &start) == -1 ) {
    C.perror( C.CString("clock gettime error 1") );
    return
  }
  
	result := product(matrix[0:nelts*nelts], vector[0:nelts], nelts)
  
  if( C.clock_gettime( C.CLOCK_MONOTONIC_RAW, &stop) == -1 ) {
    C.perror( C.CString("clock gettime error 1") );
    return
  }
  
  accum = C.double( C.long(stop.tv_sec) - C.long(start.tv_sec) ) + C.double(( C.long(stop.tv_nsec) - C.long(start.tv_nsec))) / C.double(1e9);
   
  file, err := os.OpenFile("./measurements.txt", os.O_WRONLY|os.O_CREATE|os.O_APPEND, 0666)
  
  if err != nil {
      fmt.Println("File does not exists or cannot be created")
      os.Exit(1)
  }
  
  w := bufio.NewWriter(file)
  
  // Lang, Problem, rows, cols, thresh, winnow_nelts, jobs, time
  fmt.Fprintf(w, "Go    ,Product,     ,     ,   ,%5d,%2d,%.9f,isBench:%t\n", nelts, runtime.GOMAXPROCS(0), accum, *is_bench )
  
  w.Flush()
  file.Close()

	if !*is_bench {
		fmt.Printf("%d\n", nelts)
		for i := 0; i < nelts; i++ {
			fmt.Printf("%.4f ", result[i])
		}
		fmt.Printf("\n")
	}
}
