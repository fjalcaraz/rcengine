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
   x:c1(dato y) / y<4
->
   MODIFY x(dato x.dato+1) 
   c2(dato x.dato)
   create c3(dato x.dato)
}

END
END
