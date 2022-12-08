PACKAGE Ejemplo5

TEMPORAL CLASS al
{
  id: INTEGER
  tipo: INTEGER
}

CLASS meta
{
  tipo: INTEGER
}

CLASS gastatiempo
{

}


RULESET rset5

RULE uno NORMAL TIMED 3
{
   conj: { oa:al(tipo X) / oa.id > 0 }
->
   meta(tipo X)
   CALL ON RETRACT empty_set(conj,1)
}

END

END
