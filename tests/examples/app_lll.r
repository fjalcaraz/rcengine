PACKAGE foo

TEMPORAL CLASS ll
{
   secuencia   : INTEGER
}

TEMPORAL CLASS al
{
   secuencia   : INTEGER
   contador    : INTEGER
}

PROCEDURE mif(OBJECT, INTEGER)

CLASS d  IS_A al {}

RULESET bar

RULE B HIGH
{
   ll:ll()
   al:al()
->
   CALL ON MODIFY printf("TIEMPO LL %d\n",TIME(ll))
}


END
END
