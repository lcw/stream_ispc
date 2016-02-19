/*
 * This is a derived work based on the STREAM benchmark code of John D.
 * McCalpin which uses dynamic memory allocation and ispc.
 *
 * Please find the information about the original STREAM benchmark below.
 */

/*-----------------------------------------------------------------------*/
/* Program: STREAM                                                       */
/* Revision: $Id: stream.c,v 5.10 2013/01/17 16:01:06 mccalpin Exp mccalpin $ */
/* Original code developed by John D. McCalpin                           */
/* Programmers: John D. McCalpin                                         */
/*              Joe R. Zagar                                             */
/*                                                                       */
/* This program measures memory transfer rates in MB/s for simple        */
/* computational kernels coded in C.                                     */
/*-----------------------------------------------------------------------*/
/* Copyright 1991-2013: John D. McCalpin                                 */
/*-----------------------------------------------------------------------*/
/* License:                                                              */
/*  1. You are free to use this program and/or to redistribute           */
/*     this program.                                                     */
/*  2. You are free to modify this program for your own use,             */
/*     including commercial use, subject to the publication              */
/*     restrictions in item 3.                                           */
/*  3. You are free to publish results obtained from running this        */
/*     program, or from works that you derive from this program,         */
/*     with the following limitations:                                   */
/*     3a. In order to be referred to as "STREAM benchmark results",     */
/*         published results must be in conformance to the STREAM        */
/*         Run Rules, (briefly reviewed below) published at              */
/*         http://www.cs.virginia.edu/stream/ref.html                    */
/*         and incorporated herein by reference.                         */
/*         As the copyright holder, John McCalpin retains the            */
/*         right to determine conformity with the Run Rules.             */
/*     3b. Results based on modified source code or on runs not in       */
/*         accordance with the STREAM Run Rules must be clearly          */
/*         labelled whenever they are published.  Examples of            */
/*         proper labelling include:                                     */
/*           "tuned STREAM benchmark results"                            */
/*           "based on a variant of the STREAM benchmark code"           */
/*         Other comparable, clear, and reasonable labelling is          */
/*         acceptable.                                                   */
/*     3c. Submission of results to the STREAM benchmark web site        */
/*         is encouraged, but not required.                              */
/*  4. Use of this program or creation of derived works based on this    */
/*     program constitutes acceptance of these licensing restrictions.   */
/*  5. Absolutely no warranty is expressed or implied.                   */
/*-----------------------------------------------------------------------*/


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <float.h>
#include <limits.h>
#include <assert.h>

#include "wtime.h"

#ifdef __GNUC__
#  define UNUSED __attribute__((__unused__))
#else
#  define UNUSED
#endif

#ifndef STREAM_ARRAY_SIZE
#define STREAM_ARRAY_SIZE 10000000
#endif

#ifdef NTIMES
#if NTIMES <= 1
#define NTIMES 10
#endif
#endif
#ifndef NTIMES
#define NTIMES 10
#endif

#define HLINE "-------------------------------------------------------------\n"

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef STREAM_TYPE
#define STREAM_TYPE double
#endif

#define STREAM_IS_ALIGNED(p, a) (((intptr_t)(p) & ((a)-1)) == 0)

static size_t stream_page_size()
{
  return (size_t)sysconf(_SC_PAGE_SIZE);
}

#if defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>

static size_t stream_cache_line_size()
{
  size_t line_size = 0;
  size_t sizeof_line_size = sizeof(line_size);
  sysctlbyname("hw.cachelinesize", &line_size, &sizeof_line_size, 0, 0);
  return line_size;
}

#elif defined(__linux__)

static size_t stream_cache_line_size()
{
  return (size_t)sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
}

#else
#error Unrecognized platform for cache line size
#endif

static void *stream_malloc_aligned_cache_line(size_t size, size_t line,
                                            size_t line_size, size_t page_size)
{
  void *r;
  intptr_t a;

  assert(page_size >= 1);
  assert(page_size >= line * line_size);

  r = malloc(size + sizeof(intptr_t) + page_size);

  a = (((intptr_t)r + sizeof(intptr_t) + page_size - line * line_size - 1) /
       page_size) *
          page_size +
      line * line_size;

  ((intptr_t *)a)[-1] = (intptr_t)r;

  r = (void *)a;

  return r;
}

void *stream_malloc_aligned(size_t size)
{
  void *r;
  static size_t line_no = 0;

  const size_t line_size = stream_cache_line_size();
  const size_t page_size = stream_page_size();
  const size_t line_count = page_size / line_size;

  r = stream_malloc_aligned_cache_line(size, line_no, line_size, page_size);

  assert(STREAM_IS_ALIGNED(r, 64u));

  line_no = (line_no + 1) % line_count;

  return r;
}

void stream_free_aligned(void *ptr)
{
  ptr = (void *)((intptr_t *)ptr)[-1];
  assert(ptr != NULL);
  free(ptr);
}

static double avgtime[4] = {0}, maxtime[4] = {0},
              mintime[4] = {DBL_MAX, DBL_MAX, DBL_MAX, DBL_MAX};

static char *label[4] = {"Copy:      ", "Scale:     ", "Add:       ",
                         "Triad:     "};

static double bytes[4] = {2 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE,
                          2 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE,
                          3 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE,
                          3 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE};

void checkSTREAMresults(STREAM_TYPE *a, STREAM_TYPE *b, STREAM_TYPE *c);

void stream_init_tasks(STREAM_TYPE a[], STREAM_TYPE b[], STREAM_TYPE c[],
                       int count);
void stream_selfscale_tasks(STREAM_TYPE a[], STREAM_TYPE scalar, int count);

void stream_copy_tasks(STREAM_TYPE a[], STREAM_TYPE b[], int count);
void stream_scale_tasks(STREAM_TYPE a[], STREAM_TYPE b[], STREAM_TYPE scalar,
                       int count);
void stream_add_tasks(STREAM_TYPE a[], STREAM_TYPE b[], STREAM_TYPE c[], int
    count);
void stream_triad_tasks(STREAM_TYPE a[], STREAM_TYPE b[], STREAM_TYPE c[],
    STREAM_TYPE scalar, int count);

void stream_copy(STREAM_TYPE a[], STREAM_TYPE b[], size_t count)
{
#pragma omp parallel for
  for (size_t j=0; j<count; j++)
    a[j] = b[j];
}

void stream_scale(STREAM_TYPE a[], STREAM_TYPE b[], STREAM_TYPE scalar,
    size_t count)
{
#pragma omp parallel for
  for (size_t j=0; j<count; j++)
    a[j] = scalar*b[j];
}

void stream_add(STREAM_TYPE a[], STREAM_TYPE b[], STREAM_TYPE c[],
                       size_t count)
{
#pragma omp parallel for
  for (size_t j=0; j<count; j++)
    a[j] = b[j]+c[j];
}

void stream_triad(STREAM_TYPE a[], STREAM_TYPE b[], STREAM_TYPE c[],
    STREAM_TYPE scalar, size_t count)
{
#pragma omp parallel for
  for (size_t j=0; j<count; j++)
    a[j] = b[j]+scalar*c[j];
}


int main(int UNUSED argc, char UNUSED *argv[])
{
  STREAM_TYPE scalar;
  double t, times[4][NTIMES];

  STREAM_TYPE *a, *b, *c;

#ifdef USE_POSIX_MEMALIGN
#define ALIGNMENT 4096u
  int error = posix_memalign((void**)&a, ALIGNMENT, STREAM_ARRAY_SIZE * sizeof(STREAM_TYPE));
  if (error != 0) {
    perror("posix_memalign");
    exit(EXIT_FAILURE);
  }

  error = posix_memalign((void**)&b, ALIGNMENT, STREAM_ARRAY_SIZE * sizeof(STREAM_TYPE));
  if (error != 0) {
    perror("posix_memalign");
    exit(EXIT_FAILURE);
  }

  error = posix_memalign((void**)&c, ALIGNMENT, STREAM_ARRAY_SIZE * sizeof(STREAM_TYPE));
  if (error != 0) {
    perror("posix_memalign");
    exit(EXIT_FAILURE);
  }
#else
  a = stream_malloc_aligned(STREAM_ARRAY_SIZE * sizeof(STREAM_TYPE));
  b = stream_malloc_aligned(STREAM_ARRAY_SIZE * sizeof(STREAM_TYPE));
  c = stream_malloc_aligned(STREAM_ARRAY_SIZE * sizeof(STREAM_TYPE));
#endif

  printf("Array size = %llu (elements)\n",
         (unsigned long long)STREAM_ARRAY_SIZE);
  printf("Memory per array = %.1f MiB (= %.1f GiB).\n",
         sizeof(STREAM_TYPE) * ((double)STREAM_ARRAY_SIZE / 1024.0 / 1024.0),
         sizeof(STREAM_TYPE) *
             ((double)STREAM_ARRAY_SIZE / 1024.0 / 1024.0 / 1024.0));
  printf("Total memory required = %.1f MiB (= %.1f GiB).\n",
         (3.0 * sizeof(STREAM_TYPE)) *
             ((double)STREAM_ARRAY_SIZE / 1024.0 / 1024.),
         (3.0 * sizeof(STREAM_TYPE)) *
             ((double)STREAM_ARRAY_SIZE / 1024.0 / 1024. / 1024.));
#ifdef CHUNK
  printf("Chunk size: %ju\n", (uintmax_t) CHUNK);
#endif
  printf("Page size: %ju\n", (uintmax_t) stream_page_size());
  printf("Cache line size: %ju\n", (uintmax_t) stream_cache_line_size());
  printf("sizeof(STREAM_TYPE): %ju\n", (uintmax_t) sizeof(STREAM_TYPE));

  printf("Each kernel will be executed %d times.\n", NTIMES);
  printf(" The *best* time for each kernel (excluding the first iteration)\n");
  printf(" will be used to compute the reported bandwidth.\n");

  printf(HLINE);

  if (STREAM_ARRAY_SIZE > INT_MAX)
  {
    fprintf(stderr, "Array size too big for int index\n");
    exit(EXIT_FAILURE);
  }

#ifndef DONT_USE_ISPC
  stream_init_tasks(a, b, c, STREAM_ARRAY_SIZE);
#else
#pragma omp parallel for
  for (size_t j = 0; j < STREAM_ARRAY_SIZE; ++j)
  {
    a[j] = 1.0;
    b[j] = 2.0;
    c[j] = 0.0;
  }
#endif

  printf(HLINE);

  t = wtime();
#ifndef DONT_USE_ISPC
  stream_selfscale_tasks(a, 2.0, STREAM_ARRAY_SIZE);
#else
#pragma omp parallel for
  for (size_t j = 0; j < STREAM_ARRAY_SIZE; j++)
    a[j] = 2.0E0 * a[j];
#endif
  t = 1.0E6 * (wtime() - t);

  printf("Each test below will take on the order"
         " of %d microseconds.\n",
         (int)t);

  printf(HLINE);

  /* --- MAIN LOOP --- repeat test cases NTIMES times --- */

  scalar = 3.0;
  for (int k = 0; k < NTIMES; k++)
  {
    times[0][k] = wtime();
#ifndef DONT_USE_ISPC
    stream_copy_tasks(c, a, STREAM_ARRAY_SIZE);
#else
    stream_copy(c, a, STREAM_ARRAY_SIZE);
#endif
    times[0][k] = wtime() - times[0][k];

    times[1][k] = wtime();
#ifndef DONT_USE_ISPC
    stream_scale_tasks(b, c, scalar, STREAM_ARRAY_SIZE);
#else
    stream_scale(b, c, scalar, STREAM_ARRAY_SIZE);
#endif
    times[1][k] = wtime() - times[1][k];

    times[2][k] = wtime();
#ifndef DONT_USE_ISPC
    stream_add_tasks(c, a, b, STREAM_ARRAY_SIZE);
#else
    stream_add(c, a, b, STREAM_ARRAY_SIZE);
#endif
    times[2][k] = wtime() - times[2][k];

    times[3][k] = wtime();
#ifndef DONT_USE_ISPC
    stream_triad_tasks(a, b, c, scalar, STREAM_ARRAY_SIZE);
#else
    stream_triad(a, b, c, scalar, STREAM_ARRAY_SIZE);
#endif
    times[3][k] = wtime() - times[3][k];
  }

  /* --- SUMMARY --- */

  for (int k = 1; k < NTIMES; k++) /* note -- skip first iteration */
  {
    for (int j = 0; j < 4; j++)
    {
      avgtime[j] = avgtime[j] + times[j][k];
      mintime[j] = MIN(mintime[j], times[j][k]);
      maxtime[j] = MAX(maxtime[j], times[j][k]);
    }
  }

  printf(HLINE);
  printf("Function    Best Rate MB/s  Avg time     Min time     Max time\n");
  for (int j = 0; j < 4; j++)
  {
    avgtime[j] = avgtime[j] / (double)(NTIMES - 1);

    printf("%s%12.1f  %11.6f  %11.6f  %11.6f\n", label[j],
           1.0E-06 * bytes[j] / mintime[j], avgtime[j], mintime[j], maxtime[j]);
  }

  printf(HLINE);
  /* --- Check Results --- */
  checkSTREAMresults(a, b, c);
  printf(HLINE);

#ifdef USE_POSIX_MEMALIGN
  free(a);
  free(b);
  free(c);
#else
  stream_free_aligned(a);
  stream_free_aligned(b);
  stream_free_aligned(c);
#endif

  return 0;
}

#ifndef abs
#define abs(a) ((a) >= 0 ? (a) : -(a))
#endif
void checkSTREAMresults(STREAM_TYPE *a, STREAM_TYPE *b, STREAM_TYPE *c)
{
  STREAM_TYPE aj, bj, cj, scalar;
  STREAM_TYPE aSumErr, bSumErr, cSumErr;
  STREAM_TYPE aAvgErr, bAvgErr, cAvgErr;
  double epsilon;
  ssize_t j;
  int k, ierr, err;

  /* reproduce initialization */
  aj = 1.0;
  bj = 2.0;
  cj = 0.0;
  /* a[] is modified during timing check */
  aj = 2.0E0 * aj;
  /* now execute timing loop */
  scalar = 3.0;
  for (k = 0; k < NTIMES; k++)
  {
    cj = aj;
    bj = scalar * cj;
    cj = aj + bj;
    aj = bj + scalar * cj;
  }

  /* accumulate deltas between observed and expected results */
  aSumErr = 0.0;
  bSumErr = 0.0;
  cSumErr = 0.0;
  for (j = 0; j < STREAM_ARRAY_SIZE; j++)
  {
    aSumErr += abs(a[j] - aj);
    bSumErr += abs(b[j] - bj);
    cSumErr += abs(c[j] - cj);
    // if (j == 417) printf("Index 417: c[j]: %f, cj: %f\n",c[j],cj);	//
    // MCCALPIN
  }
  aAvgErr = aSumErr / (STREAM_TYPE)STREAM_ARRAY_SIZE;
  bAvgErr = bSumErr / (STREAM_TYPE)STREAM_ARRAY_SIZE;
  cAvgErr = cSumErr / (STREAM_TYPE)STREAM_ARRAY_SIZE;

  if (sizeof(STREAM_TYPE) == 4)
  {
    epsilon = 1.e-6;
  }
  else if (sizeof(STREAM_TYPE) == 8)
  {
    epsilon = 1.e-13;
  }
  else
  {
    printf("WEIRD: sizeof(STREAM_TYPE) = %lu\n", sizeof(STREAM_TYPE));
    epsilon = 1.e-6;
  }

  err = 0;
  if (abs(aAvgErr / aj) > epsilon)
  {
    err++;
    printf("Failed Validation on array a[], AvgRelAbsErr > epsilon (%e)\n",
           epsilon);
    printf("     Expected Value: %e, AvgAbsErr: %e, AvgRelAbsErr: %e\n", aj,
           aAvgErr, abs(aAvgErr) / aj);
    ierr = 0;
    for (j = 0; j < STREAM_ARRAY_SIZE; j++)
    {
      if (abs(a[j] / aj - 1.0) > epsilon)
      {
        ierr++;
#ifdef VERBOSE
        if (ierr < 10)
        {
          printf("         array a: index: %ld, expected: %e, observed: %e, "
                 "relative error: %e\n",
                 j, aj, a[j], abs((aj - a[j]) / aAvgErr));
        }
#endif
      }
    }
    printf("     For array a[], %d errors were found.\n", ierr);
  }
  if (abs(bAvgErr / bj) > epsilon)
  {
    err++;
    printf("Failed Validation on array b[], AvgRelAbsErr > epsilon (%e)\n",
           epsilon);
    printf("     Expected Value: %e, AvgAbsErr: %e, AvgRelAbsErr: %e\n", bj,
           bAvgErr, abs(bAvgErr) / bj);
    printf("     AvgRelAbsErr > Epsilon (%e)\n", epsilon);
    ierr = 0;
    for (j = 0; j < STREAM_ARRAY_SIZE; j++)
    {
      if (abs(b[j] / bj - 1.0) > epsilon)
      {
        ierr++;
#ifdef VERBOSE
        if (ierr < 10)
        {
          printf("         array b: index: %ld, expected: %e, observed: %e, "
                 "relative error: %e\n",
                 j, bj, b[j], abs((bj - b[j]) / bAvgErr));
        }
#endif
      }
    }
    printf("     For array b[], %d errors were found.\n", ierr);
  }
  if (abs(cAvgErr / cj) > epsilon)
  {
    err++;
    printf("Failed Validation on array c[], AvgRelAbsErr > epsilon (%e)\n",
           epsilon);
    printf("     Expected Value: %e, AvgAbsErr: %e, AvgRelAbsErr: %e\n", cj,
           cAvgErr, abs(cAvgErr) / cj);
    printf("     AvgRelAbsErr > Epsilon (%e)\n", epsilon);
    ierr = 0;
    for (j = 0; j < STREAM_ARRAY_SIZE; j++)
    {
      if (abs(c[j] / cj - 1.0) > epsilon)
      {
        ierr++;
#ifdef VERBOSE
        if (ierr < 10)
        {
          printf("         array c: index: %ld, expected: %e, observed: %e, "
                 "relative error: %e\n",
                 j, cj, c[j], abs((cj - c[j]) / cAvgErr));
        }
#endif
      }
    }
    printf("     For array c[], %d errors were found.\n", ierr);
  }
  if (err == 0)
  {
    printf("Solution Validates: avg error less than %e on all three arrays\n",
           epsilon);
  }
#ifdef VERBOSE
  printf("Results Validation Verbose Results: \n");
  printf("    Expected a(1), b(1), c(1): %f %f %f \n", aj, bj, cj);
  printf("    Observed a(1), b(1), c(1): %f %f %f \n", a[1], b[1], c[1]);
  printf("    Rel Errors on a, b, c:     %e %e %e \n", abs(aAvgErr / aj),
         abs(bAvgErr / bj), abs(cAvgErr / cj));
#endif
}
