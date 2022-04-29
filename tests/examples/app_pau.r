PACKAGE foo

WINDOW=600

CLASS z
{
  zz: INTEGER
}

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

CLASS e RESTRICTS a
{
        re   = x / x=4
}

CLASS f RESTRICTS a
{
        re   = x / x=5
}
RULESET bar

RULE A  NORMAL TIMED
{
   b(id x, ruta r)
   conj:{c(id x)}
UNTIMED   [con:{d(id x)}]
          / count(conj)+count(con) = 5
     [{e(id x)}]
     [f(ruta r)]
   ->
   z()
}

END
END
