PACKAGE Func2

CLASS a
{
  atr1: STRING
}

CLASS b
{
  atr1: FLOAT
}

CLASS c
{
  atr1: INTEGER
}

RULESET RSFunc2

RULE uno NORMAL
{
  b(atr1 X) 
->
  a(atr1 floattostr(X))
  CALL printf("Valor de b(x): %s\n",floattostr(X))
  CALL printf("Valor de b(x) entero: %d\n",floattonum(X))
}

RULE dos NORMAL
{
  a(atr1 Y)
->
  CALL printf("String generado: %s\n", Y)
  c(atr1 strtonum(Y))
}

END; RULESET

END; PACKAGE
