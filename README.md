# Stanford CS 149 assignments

<https://gfxcourses.stanford.edu/cs149/fall25>

## Assignment 1

See the [`prog1_mandelbrot_threads`](prog1_mandelbrot_threads/) folder.

This assignment parallelizes the creation of an image of the Mandelbrot set, across threads, using
C++.

* [`mandelbrotSerial.cpp`](prog1_mandelbrot_threads/mandelbrotSerial.cpp) has the code to compute
  Mandelbrot brightness for a certain number of rows.
* [`mandelbrotThread.cpp`](prog1_mandelbrot_threads/mandelbrotThread.cpp) handles the threads and
  parallelization.

## Setup

First, run `brew install imagemagick` - this is for converting the `ppm` image that is outputted
into `png`.

`cd` into `prog1_mandelbrot_threads`. Run `make`. 

## Usage

Run `./mandelbrot` with the following command line arguments:

* Use `-t`/`--threads` to set the number of worker threads. 
* `-s`/`--schedule` determines how the 1,200 image rows are divided among the threads. With four
  threads, for example:
    * "block", the simpler method, gives the first 4th of rows to one thread, the second fourth to
      another thread, and so on.
    * "interleaved", the default, speedier method, gives all the rows "mod 0" to one thread, all the
      rows "mod 1" to another thread, and so on.

## Results

Running `./mandelbrot -t 4 -s block` gives:

```
(py3_12) seth@Seths-MacBook-Pro-3 prog1_mandelbrot_threads % ./mandelbrot -t 4 -s block
[mandelbrot serial]:            [236.987] ms
Wrote image file mandelbrot-serial.ppm
Thread 3 took 26.099 ms
Thread 0 took 26.206 ms
Thread 1 took 100.810 ms
Thread 2 took 101.354 ms
Thread 3 took 25.130 ms
Thread 0 took 25.418 ms
Thread 1 took 100.018 ms
Thread 2 took 100.608 ms
Thread 0 took 24.968 ms
Thread 3 took 25.181 ms
Thread 1 took 99.799 ms
Thread 2 took 100.638 ms
Thread 0 took 25.106 ms
Thread 3 took 25.225 ms
Thread 1 took 99.725 ms
Thread 2 took 100.448 ms
Thread 0 took 25.087 ms
Thread 3 took 25.200 ms
Thread 1 took 102.742 ms
Thread 2 took 102.913 ms
[mandelbrot thread]:            [100.485] ms
Wrote image file mandelbrot-thread.ppm
                                (2.36x speedup from 4 threads)
```                            

Running `./mandelbrot -t 4 -s interleaved` gives:

```
[mandelbrot serial]:            [236.337] ms
Wrote image file mandelbrot-serial.ppm
Thread 0 took 67.134 ms
Thread 2 took 67.133 ms
Thread 3 took 67.151 ms
Thread 1 took 67.663 ms
Thread 2 took 67.781 ms
Thread 3 took 67.972 ms
Thread 1 took 68.187 ms
Thread 0 took 68.542 ms
Thread 3 took 67.655 ms
Thread 1 took 67.770 ms
Thread 2 took 68.323 ms
Thread 0 took 69.067 ms
Thread 1 took 67.334 ms
Thread 3 took 67.561 ms
Thread 0 took 67.786 ms
Thread 2 took 67.829 ms
Thread 2 took 67.095 ms
Thread 0 took 67.277 ms
Thread 3 took 67.484 ms
Thread 1 took 67.525 ms
[mandelbrot thread]:            [67.560] ms
Wrote image file mandelbrot-thread.ppm
                                (3.50x speedup from 4 threads)
```