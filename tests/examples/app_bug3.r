PACKAGE foo

WINDOW = 45

CLASS c
{
   dato   : INTEGER
}

CLASS c1 is_a c{}
CLASS c2 is_a c{}
CLASS d is_a c{}

RULESET bar

RULE A NORMAL
{
   conj:[{c2()}]  / count(conj) != 2
   c1()
->
   dd:d(dato dd.dato+1)
}

END
END
