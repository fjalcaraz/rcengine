PACKAGE foo

WINDOW=600

CLASS a
{
   re   : INTEGER
   id   : INTEGER
   ruta : INTEGER
}

CLASS b RESTRICTS a
{
        re   = x / x=1
}

CLASS c RESTRICTS a
{
       re   = x / x=2
}

CLASS d RESTRICTS a
{
        re   = x / x=3
}

RULESET bar

RULE A  NORMAL TIMED
{
   b(id x, ruta r)
   conj:{c(id x)}
   UNTIMED [con:{d(id x)}]
          / count(conj)+count(con) = 2
   ->
   CALL printf("OK !!!\n")
}

END
END
