PACKAGE Func1

CLASS a
{
  atr1: STRING
  atr2: STRING
}

CLASS b
{
  atr1: STRING
}

CLASS c
{
  atr1: INTEGER
}

RULESET RSFunc1

RULE funciones1 normal
{
  obja:a(atr1 X, atr2 Y) / length(X) > 5
->
  b(atr1 append(X,Y))
  b(atr1 head(X,3))
  b(atr1 substr(X,2,4))
  c(atr1 length(Y))
  c(atr1 strtonum(Y))
  c(atr1 TIME(obja))
}

RULE B normal
{
  b(atr1 X)
->
  CALL printf("Creado b: %s\n",X)
}

RULE C normal
{
  c(atr1 X)
->
  CALL printf("Creado c: %d\n",X)
}

END; RULESET

END; PACKAGE
