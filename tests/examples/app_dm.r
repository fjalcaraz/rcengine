PACKAGE foo

CLASS e
{
   st   : INTEGER
   clv  : STRING
}

CLASS f
{
}

CLASS x
{
}

RULESET bar

RULE A  NORMAL
{
   e:e(st s) / s>=0
   f:f()
   ->
   x()
   MODIFY e(st 2)
}

END
END
