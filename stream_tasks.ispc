/*
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */

#ifndef STREAM_TYPE
#define STREAM_TYPE double
#endif

#ifndef CHUNK
#define CHUNK 16384
#endif

#define int_floor_div_pos_b(a, b) (((a) - (((a) < 0) ? ((b)-1) : 0)) / (b))

task void stream_init_task(uniform STREAM_TYPE a[], uniform STREAM_TYPE b[],
                           uniform STREAM_TYPE c[], uniform int count)
{
  uniform int first = taskIndex * CHUNK;
  uniform int last = min(count, (int)((taskIndex + 1) * CHUNK));

  foreach (i = first ... last)
  {
    a[i] = 1.0;
    b[i] = 2.0;
    c[i] = 0.0;
  }
}

export void stream_init_tasks(uniform STREAM_TYPE a[], uniform STREAM_TYPE b[],
                              uniform STREAM_TYPE c[], uniform int count)
{
  uniform int nTasks = int_floor_div_pos_b((int)count + CHUNK - 1, CHUNK);
  launch[nTasks] stream_init_task(a, b, c, count);
}

task void stream_selfscale_task(uniform STREAM_TYPE a[], uniform STREAM_TYPE scalar,
                            uniform int count)
{
  uniform int first = taskIndex * CHUNK;
  uniform int last = min(count, (int)((taskIndex + 1) * CHUNK));

  foreach (i = first ... last)
  {
    a[i] = scalar * a[i];
  }
}

export void stream_selfscale_tasks(uniform STREAM_TYPE a[], uniform STREAM_TYPE scalar,
                               uniform int count)
{
  uniform int nTasks = int_floor_div_pos_b((int)count + CHUNK - 1, CHUNK);
  launch[nTasks] stream_selfscale_task(a, scalar, count);
}


task void stream_copy_task(uniform STREAM_TYPE a[], uniform STREAM_TYPE b[],
                            uniform int count)
{
  uniform int first = taskIndex * CHUNK;
  uniform int last = min(count, (int)((taskIndex + 1) * CHUNK));

  foreach (i = first ... last)
  {
    a[i] = b[i];
  }
}

export void stream_copy_tasks(uniform STREAM_TYPE a[], uniform STREAM_TYPE b[],
                               uniform int count)
{
  uniform int nTasks = int_floor_div_pos_b((int)count + CHUNK - 1, CHUNK);
  launch[nTasks] stream_copy_task(a, b, count);
}

task void stream_scale_task(uniform STREAM_TYPE a[], uniform STREAM_TYPE b[],
                            uniform STREAM_TYPE scalar, uniform int count)
{
  uniform int first = taskIndex * CHUNK;
  uniform int last = min(count, (int)((taskIndex + 1) * CHUNK));

  foreach (i = first ... last)
  {
    a[i] = scalar*b[i];
  }
}

export void stream_scale_tasks(uniform STREAM_TYPE a[], uniform STREAM_TYPE b[],
                              uniform STREAM_TYPE scalar, uniform int count)
{
  uniform int nTasks = int_floor_div_pos_b((int)count + CHUNK - 1, CHUNK);
  launch[nTasks] stream_scale_task(a, b, scalar, count);
}

task void stream_add_task(uniform STREAM_TYPE a[], uniform STREAM_TYPE b[],
                          uniform STREAM_TYPE c[], uniform int count)
{
  uniform int first = taskIndex * CHUNK;
  uniform int last = min(count, (int)((taskIndex + 1) * CHUNK));

  foreach (i = first ... last)
  {
    a[i] = b[i] + c[i];
  }
}

export void stream_add_tasks(uniform STREAM_TYPE a[], uniform STREAM_TYPE b[],
                             uniform STREAM_TYPE c[], uniform int count)
{
  uniform int nTasks = int_floor_div_pos_b((int)count + CHUNK - 1, CHUNK);
  launch[nTasks] stream_add_task(a, b, c, count);
}



task void stream_triad_task(uniform STREAM_TYPE a[], uniform STREAM_TYPE b[],
                            uniform STREAM_TYPE c[], uniform STREAM_TYPE scalar,
                            uniform int count)
{
  uniform int first = taskIndex * CHUNK;
  uniform int last = min(count, (int)((taskIndex + 1) * CHUNK));

  foreach (i = first ... last)
  {
    a[i] = b[i] + scalar*c[i];
  }
}

export void stream_triad_tasks(uniform STREAM_TYPE a[], uniform STREAM_TYPE b[],
                               uniform STREAM_TYPE c[], uniform STREAM_TYPE scalar,
                               uniform int count)
{
  uniform int nTasks = int_floor_div_pos_b((int)count + CHUNK - 1, CHUNK);
  launch[nTasks] stream_triad_task(a, b, c, scalar, count);
}
