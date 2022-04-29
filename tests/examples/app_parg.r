PACKAGE foo

WINDOW = 45

CLASS c
{
   ch     : CHAR
   cadena : STRING
   dato   : INTEGER
}

CLASS c1 IS_A C {}
CLASS c2 IS_A C {}
CLASS d  IS_A C {}

RULESET bar


RULE A HIGH TIMED
{
   uno:c1()
->
   create d()
   CALL printf("### tabla %s nos da %d\n", uno.cadena, 145)
   CALL printf("### tabla %s nos da %d\n", uno.cadena, 145)
   CALL printf("### tabla %s nos da %d\n", uno.cadena, 145)
   CALL printf("### tabla %s nos da %d\n", uno.cadena, 145)
}


END
END
