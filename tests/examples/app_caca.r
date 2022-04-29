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
   sign        : STRING
}

CLASS v
{
   umbral : INTEGER
}


CLASS z
{
   umbral : INTEGER
}

RULESET bar

RULE D  NORMAL TIMED 100
{
   ll:{ll(id i)}
   UNTIMED cat:cat(id i)
   /
 cat.umbral <= count(ll)
   ->
   CALL printf("**** HELLO\n")
   ;CREATE al(contador count(ll))
   al(contador count(ll))
}

RULE E NORMAL
{
   v:v()
   cat:cat()
   ->
   MODIFY cat(umbral v.umbral)
}


END
END
