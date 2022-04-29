PACKAGE foo

WINDOW=10

CLASS z
{
  zz: INTEGER
}

CLASS a
{
   re   : INTEGER
}

CLASS b 
{
   re   : INTEGER
}

RULESET bar

RULE A  NORMAL TIMED
{
   {a(re x)}
   b(re x)
   ->
   z()
}

END
END
