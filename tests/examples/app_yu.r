PACKAGE foo

CLASS evento
{
   st: STRING
}

CLASS z
{
   st: STRING
}

FUNCTION BD_extraer(STRING, STRING, STRING) = STRING

RULESET bar

RULE A  NORMAL TIMED
{
   evento(st t)
   ->
  CREATE z(st BD_extraer("alfa", "pepe", append(head(t,(3* INTEGER(head(t,3) = "MOR" ))), head(t,(2* INTEGER(head(t,3) !="MOR" ))))))
}

END
END
