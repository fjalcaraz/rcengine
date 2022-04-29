PACKAGE foo

CLASS ctrl_alarma
{
  secuencia     : INTEGER
  cuenta        : INTEGER
  max_gravedad  : CHAR
}


CLASS genera
{
   nllamadas   : INTEGER
   tiempo      : INTEGER
   delta       : INTEGER
}


TEMPORAL CLASS llamada
{
 miga                          : STRING
 secuencia                     : INTEGER
 segmento_a                    : INTEGER
 servicio_origen               : STRING
 fallo                         : STRING
 sec_alarma                    : INTEGER
}

CLASS alarma 
{
  secuencia             : INTEGER
  servicio              : STRING
  segmento              : INTEGER
  miga                  : STRING
  codigo                : STRING
  informacion           : STRING
}

CLASS cat_alarma
{
  codigo        : STRING
  segmento      : INTEGER
  acum_tercera  : INTEGER
}


PROCEDURE genera_llamadas(INTEGER, INTEGER, INTEGER)
FUNCTION bd_damesecuencia(INTEGER, STRING)=INTEGER

RULESET bar

RULE B NORMAL
{
   g:genera()
->
   CALL ON INSERT genera_llamadas(g.nllamadas, g.tiempo, g.delta)
}


  RULE crear_alarma_acumul LOW TIMED 1080
  {
	llamadas: { llamada(		miga		v_miga,
					fallo		v_fallo,
					servicio_origen	v_serv,
					segmento_a	v_segm )
			}

	UNTIMED cat	: cat_alarma(	codigo		v_cod,
					segmento	v_segm )
		/ (head(v_cod,3) = "AC_")
		& (tail(v_cod,7) = llamadas.fallo)
		& (cat.acum_tercera > 0)
		& (count(llamadas) >= cat.acum_tercera)

	->
	ala:alarma(	secuencia	BD_DameSecuencia(ala.secuencia,
							 "SEC_ALARMA"),
			servicio	v_serv,
			segmento	v_segm,
			miga		v_miga,
			codigo		append(	head(v_cod,7),
						llamadas.fallo ),
			informacion	llamadas.fallo )

	ctrl_alarma(	secuencia	ala.secuencia,
			cuenta		count(llamadas) )
  }

  RULE asign_nsec_llamada NORMAL
  {
    llam : llamada( secuencia 0 )
    ->
    MODIFY llam(	secuencia	BD_DameSecuencia(llam.secuencia,
                                                         "SEC_LLAMADA"))
 
  }

  RULE act_llamada_sec_alarma LOW TIMED 1080
  {
    llam:llamada(	miga		v_miga,
			servicio_origen	v_serv,
			segmento_a	v_segm)

UNTIMED ala:alarma(	codigo		v_cod,
			miga		v_miga,
			servicio	v_serv,
			segmento	v_segm )
		/ (head(v_cod,3) = "AC_")
		& (tail(v_cod,7) = llam.fallo)

    ->
    ; -- primero: actualizo la llamada
    MODIFY llam(	sec_alarma	ala.secuencia )
  }

END
END
