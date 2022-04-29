PACKAGE foo

UNTIMED CLASS negador 
{
   dato        : STRING
   dato2     : INTEGER
}

CLASS obj1
{
   dato     : STRING
   dato2     : INTEGER
}

CLASS obj2
{
   dato     : STRING
}

CLASS final
{
   dato     : STRING
}

CLASS finn
{
   dato     : STRING
}


RULESET bar

RULE C NORMAL  TIMED 1
{
	obj1(dato X, dato2 Y)
	!negador(dato X, dato2 Y)
	[obj2(dato X)]
->
   final(dato X)
}

RULE C LOW
{
	n:negador()
	finn()
->
	delete n
}

END
END
