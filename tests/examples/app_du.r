PACKAGE foo

CLASS e
{
   st   : INTEGER
   clv  : STRING
}

CLASS x
{
}

RULESET bar

RULE A  NORMAL
{
   ! e(st 2, clv c)
   e:e(st 0, clv c)
   ->
   CALL printf("YA ESTA\n")
   MODIFY e(st 2)
}

END
END
