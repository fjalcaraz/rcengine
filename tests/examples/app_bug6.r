PACKAGE foo

WINDOW = 45

CLASS c
{
   dato   : INTEGER
}

CLASS c1 is_a c{}
CLASS c2 is_a c{}
CLASS c3 is_a c{}

RULESET bar

RULE A NORMAL
{
   c1()
->
   c2()
   delete 1
}

RULE B NORMAL
{
   c1()
->
   c3()
   delete 1
}
END
END
