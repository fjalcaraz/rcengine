PACKAGE PRUEBAS_LLAMADAS


TEMPORAL CLASS llamada
{
	miga:	STRING
	secuencia: INTEGER
	sec_alarma_acum: INTEGER
	sec_alarma_dens: INTEGER
	fallo: STRING
        tecnologia: STRING
}



ABSTRACT CLASS categoria_alarma
{
   fallo: STRING
   primera: INTEGER
   segunda: INTEGER
   tercera: INTEGER
   tecnologia: STRING
   version: STRING
}


CLASS categoria_dens IS_A categoria_alarma
{
}


CLASS alarma
{
	secuencia: INTEGER
	gravedad: CHAR
	miga: STRING
	informacion: STRING
	fecha_activacion: STRING
	codigo: STRING
}





CLASS alarma_dens RESTRICTS alarma
{
   codigo = c / head(c, 3) = "DS_"
}



CLASS control_alarma
{
   alarma: INTEGER
   llamadas: INTEGER
   max_gravedad: CHAR
}




FUNCTION BD_DameSecuencia ( INTEGER, STRING ) = INTEGER




RULESET LLAMADAS

;-------------------------------------------------------------------------------
;
;	REGLAS PARA LA CREACION DE ALARMAS
;
;-------------------------------------------------------------------------------

; Agrupamos las llamadas e implicamos las alarmas. Cuando descienda el
; numero de llamadas desapareceran solas las alarmas


RULE alarma_dens NORMAL TIMED 600
{
	lls: {llamada(miga m,
					  fallo f)}
	UNTIMED c: categoria_dens(fallo f)
	/
	(c.tercera > 0) &
	(c.tercera <= count(lls))
	->
	ca: control_alarma(alarma BD_Damesecuencia(ca.alarma, "SEC_ALARMA"),
							 llamadas count(lls))
	alarma(secuencia ca.alarma,
			 miga m,
			 codigo sprintf("DS_%s", c.fallo),
			 informacion c.fallo)
}


;-------------------------------------------------------------------------------
;
;	REGLAS PARA EL CAMBIO DE GRAVEDAD DE LAS ALARMAS
;
;-------------------------------------------------------------------------------

; Reglas "autoexplicativas"




RULE asignar_gravedad_3_dens NORMAL
{
	a: alarma_dens(secuencia sec,
						informacion i)
	ca: categoria_dens(fallo i)
	co: control_alarma(alarma sec)
	/
	(co.llamadas < ca.segunda) &
	(co.llamadas >= ca.tercera)
	->
	MODIFY a(gravedad '3',
				fecha_activacion sprintf("Gravedad dens 3"))
}

RULE asignar_gravedad_2_dens NORMAL
{
	a: alarma_dens(secuencia sec,
						informacion i)
	ca: categoria_dens(fallo i)
	co: control_alarma(alarma sec)
	/
	(co.llamadas < ca.primera) &
	(co.llamadas >= ca.segunda)
	->
	MODIFY a(gravedad '2',
				fecha_activacion sprintf("Gravedad dens 2"))
}

RULE asignar_gravedad_1_dens NORMAL
{
	a: alarma_dens(secuencia sec,
						informacion i)
	ca: categoria_dens(fallo i)
	co: control_alarma(alarma sec)
	/
	(co.llamadas >= ca.primera) 
	->
	MODIFY a(gravedad '1',
				fecha_activacion sprintf("Gravedad dens 1"))
}



;-------------------------------------------------------------------------------
;
;	REGLAS PARA LA PROPAGACION Y GRABACION EN BASE DE DATOS DE LAS ALARMAS
;
;-------------------------------------------------------------------------------

; Grabamos la alarma en base de datos cuando ya tiene gravedad 
; pero su correspondiente control de alarma no tiene gravedad maxima.
; Esto se da justo cuando se acaba de generar una alarma.
; La regla tiene maxima prioridad porque la alarma se tiene que grabar en
; cuanto se crea para poder grabar clientes afectados, llamadas, etc...

RULE grabar_alarma_bd HIGH
{
	a: alarma() / a.gravedad != '\0'
	c: control_alarma(alarma a.secuencia,
							max_gravedad '\0')
	->
	CALL ON INSERT printf("Grabo alarma %i en BD\n", a.secuencia)
	MODIFY c(max_gravedad a.gravedad)
	CALL ON INSERT printf("Propago creacion de alarma %i\n", a.secuencia)
}

; Modificamos una alarma en la base de datos solo cuando
; alcanza una nueva gravedad maxima. La comparacion es al
; reves porque la gravedad maxima es '1' y la minima '3'

RULE modificar_alarma_bd NORMAL
{
	a: alarma() / (a.gravedad != '\0')
	c: control_alarma(alarma a.secuencia)
	/
	(c.max_gravedad > a.gravedad)
	->
	CALL ON INSERT printf("Modifico alarma %i en BD\n", a.secuencia)
	MODIFY c(max_gravedad a.gravedad)
	CALL ON INSERT printf("Propago modificacion de alarma %i\n", a.secuencia)
}



;-------------------------------------------------------------------------------
;
;	REGLAS PARA ASOCIACION DE LLAMADAS Y ALARMAS
;
;-------------------------------------------------------------------------------

; Estas reglas se disparan en el momento de creacion de una alarma.
; Sirve para asociar la alarma a todas las llamadas que la han provocado.


RULE asignar_alarma_dens_a_llamada NORMAL TIMED 600
{
	ll: llamada(miga m, fallo f)
	TRIGGER a: alarma_dens(miga m,
								  informacion f) / (a.gravedad != '\0')
	->
	CHANGE ll(sec_alarma_dens a.secuencia)
        CALL ON INSERT printf("Llamada %d con alarma %d\n",ll.secuencia, a.secuencia)
}

; Estas reglas se disparan cuando llega una nueva llamada.
; sirve para asociar alarmas que YA existen a nuevas llamadas.

RULE asignar_llamada_a_alarma_dens NORMAL 
{
	TRIGGER ll: llamada(miga m, fallo f)
	a: alarma_dens(miga m, informacion f)
	->
	CHANGE ll(sec_alarma_dens a.secuencia)
        CALL ON INSERT printf("Llamada %d con alarma %d\n",ll.secuencia, a.secuencia)
}

END
END
