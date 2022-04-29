PACKAGE foo

WINDOW = 45

CLASS c
{
   ch     : CHAR
   cadena : STRING
   dato   : INTEGER
    bo    : BOOLEAN
}

CLASS c1 IS_A C {}
CLASS c2 IS_A C {}
CLASS d  IS_A C {}

RULESET bar


RULE A HIGH
{
   c1:c1()
   c2:c2()
->
 CREATE d()
   CALL printf("### PASO POR AQUI CHAIN\n")
   CALL printf("### PASO POR AQUI CHAIN\n")
}


END
END
