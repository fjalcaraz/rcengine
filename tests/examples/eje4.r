PACKAGE foo

WINDOW = 60

CLASS a1
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

RULE D NORMAL
{
   a1(id x)
   [{a2(id x)}]
   ->
   CALL ON INSERT,MODIFY,RETRACT printf("*******************************\n")
   al(dato x)
}

END
END

