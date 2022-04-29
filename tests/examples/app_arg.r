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
   d(cadena sprintf("tabla %s nos da %d", uno.cadena, 145))
}

END
END
