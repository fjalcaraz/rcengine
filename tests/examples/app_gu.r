PACKAGE foo

WINDOW=10

CLASS z
{
  zz: INTEGER
}

CLASS a
{
   re   : INTEGER
   id   : INTEGER
}

CLASS b 
{
   re   : INTEGER
   id   : INTEGER
}


CLASS c 
{
   re   : INTEGER
   id   : INTEGER
}

RULESET bar

RULE A  NORMAL TIMED
{
   a(re x)
   bb:{b(re x)}
   cc:{c(re x)} 
   ->
   z()
}

END
END
