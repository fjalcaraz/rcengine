PACKAGE foo

CLASS ll
{
   secuencia   : INTEGER
}

CLASS al
{
   secuencia   : INTEGER
   contador    : INTEGER
}

PROCEDURE mif(OBJECT, INTEGER)

CLASS d  IS_A al {}

RULESET bar

RULE A NORMAL TIMED 100
{
   conj:{ll(secuencia x)}
->
   al:al(secuencia x)
}

END
END
