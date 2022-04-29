PACKAGE foo

TEMPORAL CLASS ll
{
   id          : INTEGER
}

CLASS cat
{
   id : INTEGER
   umbral : INTEGER
}

CLASS al
{
   contador    : INTEGER
}

RULESET bar

RULE D  NORMAL TIMED 100
{
   ll:{ll(id i)}
   UNTIMED cat:cat(id i)
   /
   cat.umbral >= count(ll) 
   ->
   CALL printf("******************************* HELLO BACK \n")
   CREATE al(contador count(ll))
   ;al(contador count(ll))
}

END
END
