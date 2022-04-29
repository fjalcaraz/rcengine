PACKAGE foo

TEMPORAL CLASS ll
{
   secuencia   : INTEGER
   id          : INTEGER
   miga        : STRING
}

CLASS al
{
   secuencia   : INTEGER
   contador    : INTEGER
   miga        : STRING
}

CLASS d  IS_A al {}

RULESET bar


RULE A NORMAL  TIMED 3600
{
   conj:{ll(miga x)} / count(conj)>5
->
   al:al(secuencia 1, miga x, contador count(conj))
}



END
END
