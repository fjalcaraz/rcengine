PACKAGE RETORCIDO

TEMPORAL CLASS llamada
{
	miga: STRING
}

RULESET LLAMADAS

RULE alarma_acum NORMAL TIMED 3600
{
	lls: {llamada(miga m)} / (count(lls) >= 3)
	->
	CALL ON INSERT printf("Hala la leche !!!\n")
}

END
END
