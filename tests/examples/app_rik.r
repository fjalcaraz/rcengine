PACKAGE foo

CLASS e
{
   sec   : INTEGER
}

CLASS f
{
   sec   : INTEGER
}

CLASS g
{
   sec   : INTEGER
}
CLASS h
{
   sec   : INTEGER
}

RULESET bar

RULE A  NORMAL
{
   e(sec x)  
   TRIGGER f(sec x)
   ->
   CREATE h(sec 79865)
}



END
END
