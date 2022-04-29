PACKAGE foo

CLASS c
{
   id    : INTEGER
}

CLASS b
{
   id    : INTEGER
}

CLASS a
{
   id    : INTEGER
}

RULESET bar


RULE ONE NORMAL
{
   a()
->
   b()
   c()
}

RULE TWO NORMAL
{
   a:a()
   b()
   c()
->
   MODIFY a(id a.id)
}
END
END
