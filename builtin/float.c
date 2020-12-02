#include <math.h>

// General methods ///////////////////////////////////////////////////////////////////////

$float $float$new($WORD s) {
  return $NEW($float,s);
}

void $float_init($float self, $WORD x){
  self->val = $float_fromatom(x)->val;
}

void $float_serialize($float self, $Serial$state state) {
  $val_serialize(FLOAT_ID,&self->val,state);
}

$float $float_deserialize($Serial$state state) {
 $WORD w = $val_deserialize(state);
 double x;
 memcpy(&x,&w,sizeof($WORD));
 return to$float(x);
}

$bool $float_bool($float x) {
  return to$bool(x->val != 0.0);
}

$str $float_str($float x) {
  char *s;
  asprintf(&s,"%g",x->val);
  return to$str(s);
}

struct $float$class $float$methods = {
    "$float",
    UNASSIGNED,
    ($Super$class)&$struct$methods,
    $float_init,
    $float_serialize,
    $float_deserialize,
    $float_bool,
    $float_str
};
  
$float to$float(double x) {
  $float res = malloc(sizeof(struct $float));
  res->$class = &$float$methods;
  res->val = x;
  return res;
}

double from$float($float x) {
  return x->val;
}

$float $float_fromatom($WORD a) {
  if ($ISINSTANCE(a,$int)->val) return to$float((double)(($int)a)->val);
  if ($ISINSTANCE(a,$float)->val) return ($float)a;
  if ($ISINSTANCE(a,$bool)->val) return to$float((double)(($bool)a)->val);
  if ($ISINSTANCE(a,$str)->val) {
    double x;
    int c;
    sscanf((char *)(($str)a)->str,"%lf%n",&x,&c);
    if (c==(($str)a)->nbytes)
      return to$float(x);
    else
      RAISE(($BaseException)$NEW($ValueError,to$str("float_fromatom(): invalid str literal for type float")));
  }
  fprintf(stderr,"internal error: float_fromatom: argument not of atomic type");
  exit(-1);
}

// $Real$float /////////////////////////////////////////////////////////////////////////

void $Real$float$__serialize__($Real$float self, $Serial$state state) {
  $step_serialize(self->w$Minus, state);
}

$Real$float $Real$float$__deserialize__($Serial$state state) {
   $Real$float res = $DNEW($Real$float,state);
   res->w$Minus = ($Minus)$step_deserialize(state);
   return res;
}


$float $Real$float$__add__($Real$float wit,  $float a, $float b) {
  return to$float(from$float(a) + from$float(b));
}  

$float $Real$float$__fromatom__($Real$float wit, $WORD w) {
  return $float_fromatom(w);
}

$complex $Real$float$__complx__($Real$float wit, $float a) {
  return to$complex(a->val);
}

$float $Real$float$__mul__($Real$float wit,  $float a, $float b) {
  return to$float(from$float(a) * from$float(b));
}  

$float $Real$float$__truediv__($Real$float wit,  $float a, $float b) {
  return to$float(from$float(a) / from$float(b));
}  

$float $Real$float$__pow__($Real$float wit,  $float a, $float b) {
  return to$float(exp(from$float(b) * log(from$float(a))));
  }

$float $Real$float$__neg__($Real$float wit, $float a) {
  return to$float(-from$float(a));
}

$float $Real$float$__pos__($Real$float wit, $float a) {
  return a;
}

$WORD $Real$float$real($Real$float wit, $Real wit2, $float a) {
  return wit2->$class->__fromatom__(wit2,a);
}

$WORD $Real$float$imag($Real$float wit, $Real wit2,  $float a) {
  return wit2->$class->__fromatom__(wit2,to$float(0.0));
}

$WORD $Real$float$__abs__($Real$float wit, $Real wit2,  $float a) {
  return wit2->$class->__fromatom__(wit2,to$float(fabs(from$float(a))));
}

$float $Real$float$conjugate($Real$float wit,  $float a) {
  return a;
}
$float $Real$float$__float__ ($Real$float wit, $float x) {
  return x;
}

$WORD $Real$float$__trunc__ ($Real$float wit, $Integral wit2, $float x) {
  return wit2->$class->__fromatom__(wit2,to$int((long)trunc(from$float(x))));
}
  
$WORD $Real$float$__floor__ ($Real$float wit, $Integral wit2, $float x) {
  return wit2->$class->__fromatom__(wit2,to$int((long)floor(from$float(x))));
}
  
$WORD $Real$float$__ceil__ ($Real$float wit, $Integral wit2, $float x) {
  return wit2->$class->__fromatom__(wit2,to$int((long)ceil(from$float(x))));
}
  
$float $Real$float$__round__ ($Real$float wit, $float x, $int p) {
  double pval = p==NULL ? 0.0 : (double)p->val;
  double p10 = pow(10.0,pval);
  return to$float(round(x->val * p10)/p10);
}
     
// $Minus$float  ////////////////////////////////////////////////////////////////////////////////////////

void $Minus$float$__serialize__($Minus$float self, $Serial$state state) {
  $step_serialize(self->w$Real, state);
}

$Minus$float $Minus$float$__deserialize__($Serial$state state) {
   $Minus$float res = $DNEW($Minus$float,state);
   res->w$Real = ($Real)$step_deserialize(state);
   return res;
}

$float $Minus$float$__sub__($Minus$float wit,  $float a, $float b) {
  return to$float(from$float(a) - from$float(b));
}  

// $Ord$float  ////////////////////////////////////////////////////////////////////////////////////////

void $Ord$float$__serialize__($Ord$float self, $Serial$state state) {
}

$Ord$float $Ord$float$__deserialize__($Serial$state state) {
   $Ord$float res = $DNEW($Ord$float,state);
   return res;
}

$bool $Ord$float$__eq__ ($Ord$float wit, $float a, $float b) {
  return to$bool(a->val == b->val);
}

$bool $Ord$float$__ne__ ($Ord$float wit, $float a, $float b) {
  return to$bool(a->val != b->val);
}

$bool $Ord$float$__lt__ ($Ord$float wit, $float a, $float b) {
  return to$bool(a->val < b->val);
}

$bool $Ord$float$__le__ ($Ord$float wit, $float a, $float b) {
  return to$bool(a->val <= b->val);
}

$bool $Ord$float$__gt__ ($Ord$float wit, $float a, $float b) {
  return to$bool(a->val > b->val);
}

$bool $Ord$float$__ge__ ($Ord$float wit, $float a, $float b) {
  return to$bool(a->val == b->val);
}


// $Hashable$float ///////////////////////////////////////////////////////////////////////////////////////////////////////

void $Hashable$float$__serialize__($Hashable$float self, $Serial$state state) {
}

$Hashable$float $Hashable$float$__deserialize__($Serial$state state) {
   $Hashable$float res = $DNEW($Hashable$float,state);
   return res;
}

$bool $Hashable$float$__eq__($Hashable$float wit, $float a, $float b) {
  return to$bool(a->val == b->val);
}

$bool $Hashable$float$__neq__($Hashable$float wit, $float a, $float b) {
  return to$bool(a->val != b->val);
}

$int $Hashable$float$__hash__($Hashable$float wit, $float a) {
  return to$int($float_hash(a));
}

// init methods ////////////////////////////////////////////////////////////////////////////////////////////////

void $Real$float_init($Real$float wit) {
  wit-> w$Minus = ($Minus)$NEW($Minus$float,($Real)wit);
};

void $Minus$float_init($Minus$float wit, $Real w$Real) {
  wit->w$Real =  w$Real;
}

void $Ord$float_init($Ord$float wit) {
  return;
}

void $Hashable$float_init($Hashable$float wit) {
  return;
}

$Real$float $Real$float$new() {
  return $NEW($Real$float);
}

$Minus$float $Minus$float$new($Real wit) {
  return $NEW($Minus$float,wit);
}
  
$Ord$float $Ord$float$new() {
  return $NEW($Ord$float);
}

$Hashable$float $Hashable$float$new() {
  return $NEW($Hashable$float);
}


 struct $Real$float $Real$float_instance;
 struct $Minus$float $Minus$float_instance;
 struct $Ord$float $Ord$float_instance;
 struct $Hashable$float $Hashable$float_instance;

struct $Real$float$class $Real$float$methods = {
    "$Real$float",
    UNASSIGNED,
    ($Super$class)&$Real$methods,
    $Real$float_init,
    $Real$float$__serialize__,
    $Real$float$__deserialize__,
    ($bool (*)($Real$float))$default__bool__,
    ($str (*)($Real$float))$default__str__,
    $Real$float$__add__,
    $Real$float$__fromatom__,
    $Real$float$__complx__,
    $Real$float$__mul__,
    $Real$float$__truediv__,
    $Real$float$__pow__,
    $Real$float$__neg__,
    $Real$float$__pos__,
    $Real$float$real,
    $Real$float$imag,
    $Real$float$__abs__,
    $Real$float$conjugate,
    $Real$float$__float__,
    $Real$float$__trunc__ ,
    $Real$float$__floor__ ,
    $Real$float$__ceil__ ,
    $Real$float$__round__
};
struct $Real$float $Real$float_instance = {&$Real$float$methods, ($Minus)&$Minus$float_instance};
$Real$float $Real$float$witness = &$Real$float_instance;

struct $Minus$float$class $Minus$float$methods = {
    "$Minus$float",
    UNASSIGNED,
    ($Super$class)&$Minus$methods,
    $Minus$float_init,
    $Minus$float$__serialize__,
    $Minus$float$__deserialize__,
    ($bool (*)($Minus$float))$default__bool__,
    ($str (*)($Minus$float))$default__str__,
    $Minus$float$__sub__
};
struct $Minus$float $Minus$float_instance = {&$Minus$float$methods, ($Real)&$Real$float_instance};
$Minus$float $Minus$float$witness = &$Minus$float_instance;


struct $Ord$float$class $Ord$float$methods = {
    "$Ord$float",
    UNASSIGNED,
    ($Super$class)&$Ord$methods,
    $Ord$float_init,
    $Ord$float$__serialize__,
    $Ord$float$__deserialize__,
    ($bool (*)($Ord$float))$default__bool__,
    ($str (*)($Ord$float))$default__str__,
    $Ord$float$__eq__ ,
    $Ord$float$__ne__ ,
    $Ord$float$__lt__ ,
    $Ord$float$__le__ ,
    $Ord$float$__gt__ ,
    $Ord$float$__ge__
};
struct $Ord$float $Ord$float_instance = {&$Ord$float$methods};
$Ord$float $Ord$float$witness = &$Ord$float_instance;

struct $Hashable$float$class $Hashable$float$methods = {
    "$Hashable$float",
    UNASSIGNED,
    ($Super$class)&$Hashable$methods,
    $Hashable$float_init,
    $Hashable$float$__serialize__,
    $Hashable$float$__deserialize__,
    ($bool (*)($Hashable$float))$default__bool__,
    ($str (*)($Hashable$float))$default__str__,
    $Hashable$float$__eq__,
    $Hashable$float$__neq__,
    $Hashable$float$__hash__
};
struct $Hashable$float $Hashable$float_instance = {&$Hashable$float$methods};
$Hashable$float $Hashable$float$witness = &$Hashable$float_instance;
 
