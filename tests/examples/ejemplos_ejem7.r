PACKAGE Ejemplo7

TEMPORAL CLASS a
{
  id: INTEGER
  control: INTEGER
}

CLASS b
{
  id: INTEGER
}

RULESET reglas7

RULE A NORMAL TIMED 5
{
  a(id X)
->
  b(id X)
}

END

END
