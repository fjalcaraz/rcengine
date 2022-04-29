PACKAGE foo

WINDOW=30

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

CLASS z
{
   co   : INTEGER
   id   : INTEGER
}

RULESET bar

RULE A  NORMAL  TIMED
{
   a(co x)
   conj:{b:b(co x)}
UNTIMED    con:{c(co x)}
          / count(conj) + count(con)  = 5
   ->
   z(id x)
}

END
END
