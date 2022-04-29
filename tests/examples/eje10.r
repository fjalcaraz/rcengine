PACKAGE foo

WINDOW = 60

TEMPORAL CLASS a1
{
   id          : INTEGER
}

CLASS a2
{
   id : INTEGER
   dato : INTEGER
}

CLASS al
{
   dato    : INTEGER
}

RULESET bar

RULE D NORMAL TIMED 10
{
   c:{a1(id x)} / count(c) > 5
   ->
   CALL ON INSERT,MODIFY,RETRACT printf("*******************************\n")
}

END
END

