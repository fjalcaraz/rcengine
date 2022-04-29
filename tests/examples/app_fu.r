PACKAGE foo


CLASS evento
{
   st: STRING
}

CLASS z
{
   st: STRING
   id: INTEGER
}

RULESET bar

RULE A  NORMAL TIMED
{
   evento()
   ->
  ;CREATE z(st sprintf("%d",3 * INTEGER("MOR" = "MOR" )))
  ;CREATE z(id 3 * INTEGER("MOR" = "MOR" ))
   CALL printf("%d",3 * INTEGER("MOR" = "MOR" ))
}

END
END
