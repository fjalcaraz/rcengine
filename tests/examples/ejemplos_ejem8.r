PACKAGE Ejemplo8

TEMPORAL CLASS a
{
  id: INTEGER
}

CLASS b
{
  id: INTEGER
}

RULESET reglas8

RULE A NORMAL TIMED 5
{
  a(id X)
->
  b(id X)
}

END

END
