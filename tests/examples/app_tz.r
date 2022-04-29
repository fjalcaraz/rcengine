PACKAGE foo

FUNCTION fnsec(STRING, STRING, INTEGER) = STRING

WINDOW=10

CLASS z
{
  zz: INTEGER
}

CLASS a
{
   id   : INTEGER
   s1   : STRING
   s2   : STRING
}

CLASS b
{
   id   : INTEGER
   s    : STRING
}


RULESET bar

RULE A  NORMAL TIMED
{
   aa:a()
   ->
   bb:b(id aa.id, s "jua")
}

END
END
