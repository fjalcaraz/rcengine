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
  CALL printf("%s, LEN=%d, HEAD(2)=%s, BUTLAST(2)=%s TAIL(2)=%s SUBSTR(2,3)=%s\n", 
        "áéíóúÑÀ", 
        length("áéíóúÑÀ"),
        head("áéíóúÑÀ", 2),
        butlast("áéíóúÑÀ", 2),
        tail("áéíóúÑÀ", 2),
        substr("áéíóúÑÀ", 2, 3))
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
