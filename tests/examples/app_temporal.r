PACKAGE foo

TEMPORAL CLASS ll
{
   secuencia   : INTEGER
}

CLASS al
{
   secuencia   : INTEGER
   contador    : INTEGER
}

RULESET bar

RULE A NORMAL TIMED 100
{
   conj:{ll(secuencia 3)} / count(conj)>5
->
   al:al(secuencia 1, contador count(conj))
}

END
END
