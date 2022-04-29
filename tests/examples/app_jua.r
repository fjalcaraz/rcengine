PACKAGE foo

CLASS ll
{
   secuencia   : INTEGER
}

CLASS al
{
   secuencia   : INTEGER
   contador    : INTEGER
}

RULESET bar

RULE B HIGH
{
   ll:ll()
->
   CALL printf("TIEMPO\n")
}

RULE C LOW
{
   TRIGGER ll:ll()
   al:al()
->
   CALL printf("TRIGGER\n")
}
END
END
