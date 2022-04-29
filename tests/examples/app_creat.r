PACKAGE foo

WINDOW = 45

CLASS c
{
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
   d()
   create c2(dato 2)
   create c4(dato 4)
   create c3(dato 3)
}

END
END
