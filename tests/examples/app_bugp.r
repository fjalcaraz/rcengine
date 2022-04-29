PACKAGE foo

WINDOW = 45

TRIGGER CLASS t
{
   dato   : INTEGER
}

CLASS c
{
   dato   : INTEGER
}

CLASS c1 IS_A C {
   dato1: INTEGER
}

CLASS c2 IS_A c1 {
   dato2: INTEGER
}


CLASS c3 IS_A c1 {
   dato3: INTEGER
}

CLASS d  IS_A C {}

RULESET bar

RULE A NORMAL
{
   c1()
->
   CREATE t()
}

END
END
