PACKAGE foo

WINDOW=30

CLASS z
{
  zz: INTEGER
}

CLASS a
{
   id   : INTEGER
}

CLASS b
{
   id   : INTEGER
}
CLASS c
{
   id   : INTEGER
}

RULESET bar

RULE A  NORMAL
{
   {a(id x)}
   {b(id x)}
   {c(id x)}
   ->
   z()
}

END
END
