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

RULE A  NORMAL
{
   {b(ruta r, id x)}
    c(id x)
    [d(id x, ruta r)]
   ->
   z()
}

END
END
