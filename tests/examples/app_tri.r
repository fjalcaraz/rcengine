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
   e()  
   ->
   f(sec 79865)
}


RULE B  HIGH
{
   e()
   ->
   g()
}

RULE C  NORMAL
{
   g()
   ->
   h()
}

END
END
