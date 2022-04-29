PACKAGE foo

WINDOW=600

CLASS al
{
   id   : INTEGER
   ce   : STRING
   si   : INTEGER
   eqp  : STRING
   eq   : STRING
   uf   : CHAR
}

CLASS al_r RESTRICTS al
{
   id = 11
   UF = 'M'
}


CLASS al_e RESTRICTS al
{
   id = 0
   UF = 'A'
}

RULESET bar

RULE A  NORMAL TIMED
{
   al_e(ce x, si y, eq z, eqp P, uf U)
   conj:{al_r(ce x, si y,  eqp z)} / count(conj) = 3
   ->
   al(id 902, ce x, si y, eq z, eqp P, uf U)
}

END
END
