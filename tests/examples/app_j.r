PACKAGE viruta

CLASS b
{
  id :INTEGER
}
 
CLASS g
{
        id : INTEGER
}

CLASS s
{
        state : STRING
}
 
CLASS e
{
  id                : INTEGER
}

CLASS em
{
  id                : INTEGER
}

RULESET REGLAS_BACKUP

RULE A LOW
{
        emilito:em()
	evento:e()
	->
        CALL ON INSERT printf("T EMILITO %d   T EVENTO %d\n", TIME(emilito), TIME(evento))
	CREATE b()
 	CREATE g()
	CREATE s(state "backup")
}
RULE B LOW
{
	s:s(state "backup")
	b()
 	g()
	->
	MODIFY s(state "notificar_iu");
}


RULE C LOW
{
	s:s(state "notificar_iu")
	->
	MODIFY s(state "finalizar");
}
	
END

END
