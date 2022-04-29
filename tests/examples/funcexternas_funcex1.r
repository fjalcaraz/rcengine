PACKAGE FuncEx1

class a
{
  atrib: INTEGER
}

class b
{
  atrib: INTEGER
}

FUNCTION test_func(INTEGER) = INTEGER

PROCEDURE test_func2(STRING)

RULESET Funcex1rs

RULE UNO NORMAL
{
a(atrib X)
->
b(atrib test_func(X))
}

RULE DOS NORMAL
{
b(atrib X)
->
CALL test_func2(numtostr(X))
}

END ;RULESET

END ;PACKAGE
