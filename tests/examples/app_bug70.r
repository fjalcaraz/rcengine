; Compound y single no liberado .....  ya estab en CS con tag INSERT !!!
PACKAGE foo

WINDOW = 45

CLASS c
{
   ch     : CHAR
   cadena : STRING
   dato   : INTEGER
}

PROCEDURE mif(INTEGER)


CLASS c1 IS_A C {}
CLASS c2 IS_A C {}
CLASS d  IS_A C {}

RULESET bar


RULE A NORMAL TIMED
{
   uno:c1()
->
  d()
}

RULE B NORMAL TIMED
{
   uno:c1()
->
   delete 1
}

END
END
