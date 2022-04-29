PACKAGE foo

CLASS a
{
   id   : INTEGER
}

CLASS b
{
   id   : INTEGER
}

CLASS c
{
   id   : INTEGER
}

RULESET bar

RULE A  NORMAL
{
   {b:b()} / b.id = 1 | b.id=2
   ->
  a()
   CALL printf("OK !!!\n")
}

END
END
