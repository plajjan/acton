/*
 * Copyright (C) 2019-2021 Data Ductus AB
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "../../builtin/builtin.h"
#include "../ndarray.h"

int main(int argc, char *argv[]) {
  long n;
  long iters;
  sscanf(argv[1],"%ld",&n);
  sscanf(argv[2],"%ld",&iters);
  $ndarray x = $ndarray_linspace(to$float(0.0),to$float(1.0), to$int(n));
  // printf("x=%s\n",x->$class->__str__(x)->str);
  $list ix = $NEW($list,NULL,NULL);
  $Slice s = $NEW($Slice,NULL,NULL,NULL);
  $list_append(ix,s);
  $list_append(ix,NULL);
  $Plus$ndarray$float wit = $Plus$ndarray$float$witness;
  for (int i=0;i<iters; i++) {
    $ndarray r = wit->$class->__add__(wit,$nd_getslice(x,ix),x);
    //printf("r=%s\n",r->$class->__str__(r)->str);
    printf("%f\n",from$float($ndarray_sumf(r)));
    free(r->data);
    free(r);
  }
}
        
