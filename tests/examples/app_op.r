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

;CLASS c
;{
;   re   : INTEGER
;   id   : INTEGER
;   ruta : INTEGER
;}

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
   UNTIMED [con:{d(id x)}]
   conj:{c(id x)}
   [co:{e(id x)}]
   [f(ruta r)]
          / count(conj)+count(con) = 2
   ->
   CALL printf("OK !!!\n")
}

END
END
