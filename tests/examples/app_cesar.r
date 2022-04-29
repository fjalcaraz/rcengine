PACKAGE foo

WINDOW=600

CLASS a
{
   uf   : INTEGER
}

CLASS b
{
   uf   : INTEGER
}

CLASS d
{
   uf   : INTEGER
}

RULESET bar

RULE A  HIGH
{
   a()
   ->
   b()
}


RULE B  NORMAL
{
   a()
   !b()
   ->
   CALL printf("HOLA\n")
}

END
END
