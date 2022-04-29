PACKAGE foo

WINDOW = 45

CLASS c1
{
   dato   : INTEGER
}

CLASS c2
{
   dato   : INTEGER
}


CLASS c3
{
   dato   : INTEGER
}
CLASS c4
{
   dato   : INTEGER
}
RULESET bar


RULE A NORMAL
{
   c1()
   c2:c2()
->
   create c3:c3()
   modify c2(dato 6)
   ;c4(dato c3.dato)
}

RULE B NORMAL
{
c4()
c2()
->
delete 2
}

END
END
