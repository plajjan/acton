// Auxiliary //////////////////////////////////////////////////////////////////////////////

// only called with e>=0.
static long longpow(long a, long e) {
  if (e == 0) return 1;
  if (e == 1) return a;
  if (e%2 == 0) return longpow(a*a,e/2);
  return a * longpow(a*a,e/2);
}

// General methods ///////////////////////////////////////////////////////////////////////

void $int_init($int self, $WORD a){
  self->val = $int_fromatom(a)->val;
}

void $int_serialize($int n,$Serial$state state) {
  $val_serialize(INT_ID,&n->val,state);
}

$int $int_deserialize($Serial$state state) {
  return to$int((long)$val_deserialize(state));
}

$bool $int_bool($int n) {
  return to$bool(n->val != 0);
}

$str $int_str($int n) {
  char *s;
  asprintf(&s,"%ld",n->val);
  return to$str(s);
}
  
struct $int$class $int$methods = {"",UNASSIGNED,($Super$class)&$struct$methods,$int_init,$int_serialize,$int_deserialize,$int_bool,$int_str};

$int to$int(long i) {
  $int res = malloc(sizeof(struct $int));
  res->$class = &$int$methods;
  res->val = i;
  return res;
}

long from$int($int w) {
  return w->val;
}

$int $int_fromatom($Super a) {
  if ($ISINSTANCE(a,$int)) return ($int)a;
  if ($ISINSTANCE(a,$float)) return to$int(round((($float)a)->val));
  if ($ISINSTANCE(a,$bool)) return to$int((($bool)a)->val);
  if ($ISINSTANCE(a,$str)) {
    long x;
    int c;
    sscanf((char *)(($str)a)->str,"%ld%n",&x,&c);
    if (c==(($str)a)->nbytes)
      return to$int(x);
    else 
      RAISE(($BaseException)$NEW($ValueError,to$str("int_fromatom(): invalid str literal for type int")));
  }
  fprintf(stderr,"internal error: int_fromatom: argument not of atomic type");
  exit(-1);
}
                  

// $Integral$int /////////////////////////////////////////////////////////////////////////

$bool $Integral$int$__eq__ ($Integral$int wit, $int a, $int b) {
  return to$bool(a->val == b->val);
}

$bool $Integral$int$__ne__ ($Integral$int wit, $int a, $int b) {
  return to$bool(a->val != b->val);
}

$bool $Integral$int$__lt__ ($Integral$int wit, $int a, $int b) {
  return to$bool(a->val < b->val);
}

$bool $Integral$int$__le__ ($Integral$int wit, $int a, $int b) {
  return to$bool(a->val <= b->val);
}

$bool $Integral$int$__gt__ ($Integral$int wit, $int a, $int b) {
  return to$bool(a->val > b->val);
}

$bool $Integral$int$__ge__ ($Integral$int wit, $int a, $int b) {
  return to$bool(a->val == b->val);
}

$float $Integral$int$__float__ ($Integral$int wit, $int n) {
  return to$float((double)n->val);
}

$WORD $Integral$int$__trunc__ ($Integral$int wit, $Integral wit2, $int n) {
  $Number wit3 = wit2->w$Number$Integral;
  return wit3->$class->__fromatom__(wit3,n);
}
  
$WORD $Integral$int$__floor__ ($Integral$int wit, $Integral wit2, $int n) {
  $Number wit3 = wit2->w$Number$Integral;
  return wit3->$class->__fromatom__(wit3,n);
}
  
$WORD $Integral$int$__ceil__ ($Integral$int wit, $Integral wit2, $int n) {
  $Number wit3 = wit2->w$Number$Integral;
  return wit3->$class->__fromatom__(wit3,n);
}
  
$int $Integral$int$__round__ ($Integral$int wit, $int n, $int p) {
  long nval = n->val;
  if (nval<0)
    return to$int(-$Integral$int$__round__(wit,to$int(-nval),p)->val);
  long pval = p==NULL ? 0 : p->val;
  if (pval>=0)
    return n;
  long p10 = longpow(10,-pval);
  long res = nval/p10;
  if (nval%p10 * 2 > p10)
    res++; 
  return to$int (res * p10);
}
  
$WORD $Integral$int$numerator ($Integral$int wit, $Integral wit2, $int n) {
  $Number wit3 = wit2->w$Number$Integral;
  return wit3->$class->__fromatom__(wit3,n);
}
  
$WORD $Integral$int$denominator ($Integral$int wit, $Integral wit2, $int n) {
  $Number wit3 = wit2->w$Number$Integral;
  return wit3->$class->__fromatom__(wit3,to$int(1L));
}
  
$int $Integral$int$__int__ ($Integral$int wit, $int n) {
  return n;
}

$int $Integral$int$__index__($Integral$int wit, $int n) {
  return n;
}

$tuple $Integral$int$__divmod__($Integral$int wit, $int a, $int b) {
  int n = a->val;
  int d = b->val;
  $WORD *comps = malloc(2*sizeof($WORD));
  comps[0] = to$int(n/d);
  comps[1] = to$int(n%d);
  return $NEW($tuple,2,comps);
}

$int $Integral$int$__floordiv__($Integral$int wit, $int a, $int b) {
  return to$int(a->val / b->val);
}

$int $Integral$int$__mod__($Integral$int wit, $int a, $int b) {
  return to$int(a->val % b->val);
}

$int $Integral$int$__lshift__($Integral$int wit,  $int a, $int b) {
  return to$int(a->val << b->val);
}

$int $Integral$int$__rshift__($Integral$int wit,  $int a, $int b) {
  return to$int(a->val >> b->val);
}
 
$int $Integral$int$__invert__($Integral$int wit,  $int a) {
  return to$int(~a->val);
}


// Logical$int  ////////////////////////////////////////////////////////////////////////////////////////

$int $Logical$int$__and__($Logical$int wit,  $int a, $int b) {
  return to$int(a->val & b->val);
}
                                                 
$int $Logical$int$__or__($Logical$int wit,  $int a, $int b) {
  return to$int(a->val | b->val);
}
                                                 
$int $Logical$int$__xor__($Logical$int wit,  $int a, $int b) {
  return to$int(a->val ^ b->val);
}  

// $Number$int //////////////////////////////////////////////////////////////////////////////////////

$bool $Number$int$__eq__ ($Number$int wit, $int a, $int b) {
  return to$bool(a->val == b->val);
}

$bool $Number$int$__ne__ ($Number$int wit, $int a, $int b) {
  return to$bool(a->val != b->val);
}

$int $Number$int$__fromatom__($Number$int wit,$WORD w) {
  return $int_fromatom(w);
}

$complex $Number$int$__complx__($Number$int wit, $int a) {
  return to$complex((double)a->val);
}

$int $Number$int$__mul__($Number$int wit,  $int a, $int b) {
  return to$int(a->val * b->val);
}  

// The typechecker will reject true division between two integers.
$int $Number$int$__truediv__($Number$int wit,  $int a, $int b) {
  // raise NOTIMPLEMENTED
  return NULL;
}  

  
$int $Number$int$__pow__($Number$int wit,  $int a, $int b) {
  if ( b->val < 0) {
    // raise VALUEERROR;
    return NULL;
  }
  return to$int(longpow(a->val,b->val));
}

$int $Number$int$__neg__($Number$int wit,  $int a) {
  return to$int(-a->val);
}

$int $Number$int$__pos__($Number$int wit,  $int a) {
  return a;
}

$WORD $Number$int$real($Number$int wit, $Real wit2, $int a) {
  $Number wit3 = wit2->w$Number$Real;
  return wit3->$class->__fromatom__(wit3,a);
}

$WORD $Number$int$imag($Number$int wit, $Real wit2,  $int a) {
  $Number wit3 = wit2->w$Number$Real;
  return wit3->$class->__fromatom__(wit3,to$int(0L));
}

$WORD $Number$int$__abs__($Number$int wit, $Real wit2,  $int a) {
  $Number wit3 = wit2->w$Number$Real;
  return wit3->$class->__fromatom__(wit3,to$int(labs(a->val)));
}

$int $Number$int$__conjugate__($Number$int wit,  $int a) {
  return a;
}

// $Plus$int  ////////////////////////////////////////////////////////////////////////////////////////

$int $Plus$int$__add__($Plus$int wit,  $int a, $int b) {
  return to$int(a->val + b->val);
}  
 
// $Minus$int  ////////////////////////////////////////////////////////////////////////////////////////

$int $Minus$int$__sub__($Minus$int wit,  $int a, $int b) {
  return to$int(a->val - b->val);
}  

// $Hashable$int ///////////////////////////////////////////////////////////////////////////////////////////////////////

$bool $Hashable$int$__eq__($Hashable$int wit, $int a, $int b) {
  return to$bool(a->val == b->val);
}

$bool $Hashable$int$__neq__($Hashable$int wit, $int a, $int b) {
  return to$bool(a->val != b->val);
}

$int $Hashable$int$__hash__($Hashable$int wit, $int a) {
  return to$int($int_hash(a));
}

// Initialization ////////////////////////////////////////////////////////////////////////////////////////////////////////

void $Integral$int_init($Integral$int wit) {
  wit-> w$Number$Integral = $NEW($Number$int,wit);
  wit-> w$Logical$Integral = $NEW($Logical$int,wit);
};

void $Number$int_init($Number$int wit, $Integral$int w$Integral$int) {
  wit-> w$Integral$int = w$Integral$int;
  wit-> w$Plus$Number = $NEW($Plus$int,wit);
  wit-> w$Minus$Number = $NEW($Minus$int,wit);
}

void $Logical$int_init($Logical$int wit, $Integral$int w$Integral$int) {
  wit->w$Integral$int =  w$Integral$int;
}

void $Plus$int_init($Plus$int wit, $Number$int w$Number$int) {
  wit->w$Number$int =  w$Number$int;
}

void $Minus$int_init($Minus$int wit, $Number$int w$Number$int) {
  wit->w$Number$int =  w$Number$int;
}

struct $Integral$int $Integral$int_instance;
struct $Logical$int $Logical$int_instance;
struct $Number$int $Number$int_instance;
struct $Plus$int $Plus$int_instance;
struct $Minus$int $Minus$int_instance;
struct $Hashable$int $Hashable$int_instance;

struct $Integral$int$class $Integral$int$methods = {"",UNASSIGNED,NULL,$Integral$int_init, $Integral$int$__eq__ , $Integral$int$__ne__ , $Integral$int$__lt__ , $Integral$int$__le__ ,
                                                     $Integral$int$__gt__ , $Integral$int$__ge__ , $Integral$int$__float__ , $Integral$int$__trunc__ , $Integral$int$__floor__ ,
                                                     $Integral$int$__ceil__ , $Integral$int$__round__ , $Integral$int$numerator , $Integral$int$denominator ,
                                                     $Integral$int$__int__ , $Integral$int$__index__ , $Integral$int$__divmod__ , $Integral$int$__floordiv__ ,
                                                     $Integral$int$__mod__ , $Integral$int$__lshift__ , $Integral$int$__rshift__ , $Integral$int$__invert__};
struct $Integral$int $Integral$int_instance = {&$Integral$int$methods, &$Logical$int_instance, &$Number$int_instance};
$Integral$int $Integral$int$witness = &$Integral$int_instance;


struct $Logical$int$class $Logical$int$methods =  {"", UNASSIGNED,NULL,$Logical$int_init, $Logical$int$__and__ , $Logical$int$__or__ , $Logical$int$__xor__};
struct $Logical$int $Logical$int_instance = {&$Logical$int$methods, &$Integral$int_instance};
$Logical$int $Logical$int$witness = &$Logical$int_instance;


struct $Number$int$class $Number$int$methods = {"", UNASSIGNED,NULL,$Number$int_init, $Number$int$__eq__,$Number$int$__ne__,$Number$int$__fromatom__,$Number$int$__complx__,
                                               $Number$int$__mul__,$Number$int$__truediv__,$Number$int$__pow__,$Number$int$__neg__,
                                               $Number$int$__pos__,$Number$int$real,$Number$int$imag,$Number$int$__abs__,$Number$int$__conjugate__};
struct $Number$int $Number$int_instance = {&$Number$int$methods, &$Integral$int_instance, &$Plus$int_instance, &$Minus$int_instance};
$Number$int $Number$int$witness = &$Number$int_instance;

struct $Plus$int$class $Plus$int$methods = {"",UNASSIGNED,NULL,$Plus$int_init, $Plus$int$__add__};
struct $Plus$int $Plus$int_instance = {&$Plus$int$methods, &$Number$int_instance};
$Plus$int $Plus$int$witness = &$Plus$int_instance;

struct $Minus$int$class $Minus$int$methods = {"",UNASSIGNED, NULL,$Minus$int_init, $Minus$int$__sub__};
struct $Minus$int $Minus$int_instance = {&$Minus$int$methods, &$Number$int_instance};
$Minus$int $Minus$int$witness = &$Minus$int_instance;

struct $Hashable$int$class $Hashable$int$methods = {"",UNASSIGNED, NULL,(void (*)($Hashable$int))$default__init__, $Hashable$int$__eq__,$Hashable$int$__neq__,$Hashable$int$__hash__};
struct $Hashable$int $Hashable$int_instance = {&$Hashable$int$methods};
$Hashable$int $Hashable$int$witness = &$Hashable$int_instance;
