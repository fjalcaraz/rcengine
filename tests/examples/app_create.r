PACKAGE foo

WINDOW = 45

CLASS c
{
   ch     : CHAR
   cadena : STRING
   dato   : INTEGER
}

FUNCTION mif(STRING) = CHAR


CLASS c1 IS_A C {}
CLASS c2 IS_A C {}
CLASS c3 IS_A C {}
CLASS c4 IS_A C {}
CLASS d  IS_A C {}

RULESET bar


RULE A NORMAL TIMED
{
   c1()
->
   c2()
   create c3()
}

END
END
