PACKAGE foo

CLASS e
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
   ->
   CREATE h(sec 79865)
}



END
END
