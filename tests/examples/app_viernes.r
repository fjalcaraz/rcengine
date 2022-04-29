PACKAGE foo

CLASS a
{
   co   : INTEGER
   id   : INTEGER
}

CLASS b
{
   co   : INTEGER
   id   : INTEGER
}

CLASS c
{
   co   : INTEGER
   id   : INTEGER
}

CLASS d
{
   co   : INTEGER
   id   : INTEGER
}

CLASS z
{
   co   : INTEGER
   id   : INTEGER
}

CLASS m
{
   co   : INTEGER
   id   : INTEGER
}
RULESET bar

RULE A  NORMAL TIMED 10
{
   a(co x)
   conj:{b(co x)}
   UNTIMED con:{c(co x)}
          / count(conj) = 4
   ->
   z()
}

END
END
