PACKAGE ejem9

temporal class alarma
{
  id: INTEGER
  tipo: INTEGER
}

class rafaga
{
  tipo: INTEGER
}

RULESET ejem9R

RULE a NORMAL TIMED 5
{
  conj: { ala:alarma(tipo X) / ala.id > 0 } / count(conj) > 3
->
  rafaga(tipo X)
}

END ; RULESET

END ; PACKAGE
