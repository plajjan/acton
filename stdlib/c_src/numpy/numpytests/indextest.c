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

int main() {
  $ndarray v = $ndarray_arange(to$int(60));
  $list newshape = $NEW($list,NULL,NULL);
  $list_append(newshape,to$int(3));
  $list_append(newshape,to$int(4));
  $list_append(newshape,to$int(5));
  $ndarray a = $ndarray_reshape(v,newshape);
  printf("a=%s\n",from$str(a->$class->__str__(a)));
  $list ix = $NEW($list,NULL,NULL);
  $list_append(ix,NULL);
  $Slice s1 = $NEW($Slice,NULL,NULL,NULL);
  $list_append(ix,s1);
  $list_append(ix,to$int(2));
  $ndarray b = $nd_getslice(a,ix);
  printf("b=%s\n",from$str(b->$class->__str__(b)));
  $list ix1 = $NEW($list,NULL,NULL);
  $list_append(ix1,to$int(1));
  $Slice s2 = $NEW($Slice,to$int(1),NULL,NULL);
  $list_append(ix1,s2);
  $Slice s3 = $NEW($Slice,to$int(-1),to$int(0),to$int(-1));
  $list_append(ix1,s3);
  $ndarray c = $nd_getslice(a,ix1);
  printf("c=%s\n",from$str(c->$class->__str__(c)));
  $list test = $list_copy(c->shape);
  $ndarray d = $ndarray_func1(mul2,c);
  printf("d=%s\n",from$str(d->$class->__str__(d)));
  $ndarray e = $ndarray_fromatom(($Super)to$int(3));
  printf("e.shape = %s\n",e->shape->$class->__str__(e->shape)->str);
  printf("e.strides = %s\n",e->strides->$class->__str__(e->strides)->str);
  printf("e=%s\n",from$str(e->$class->__str__(e)));
  $Plus$ndarray$int wit = $Plus$ndarray$int$witness;
  $ndarray f = wit->$class->__add__(wit,d,e);
  printf("f=%s\n",from$str(f->$class->__str__(f)));  
  $ndarray g = wit->$class->__add__(wit,a,e);
  printf("g.shape = %s\n",g->shape->$class->__str__(g->shape)->str);
  printf("g.strides = %s\n",g->strides->$class->__str__(g->strides)->str);
  printf("g=%s\n",from$str(g->$class->__str__(g)));  
}
