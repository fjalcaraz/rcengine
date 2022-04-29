PACKAGE foo


TEMPORAL CLASS llamada
{
	miga:	STRING
	tecnologia: STRING
	secuencia: INTEGER
	sec_alarma_acum: INTEGER
	sec_alarma_dens: INTEGER
	segmento_a: INTEGER
	servicio_origen: STRING
	fallo: STRING
}



ABSTRACT CLASS categoria_alarma
{
   fallo: STRING
   tecnologia: STRING
   segmento: INTEGER
   primera: INTEGER
   segunda: INTEGER
   tercera: INTEGER
}

CLASS categoria_acum IS_A categoria_alarma
{
}

CLASS categoria_dens IS_A categoria_alarma
{
}


CLASS alarma
{
	secuencia: INTEGER
	gravedad: CHAR
	servicio: STRING
	segmento: INTEGER
	miga: STRING
	codigo: STRING
	informacion: STRING
	fecha_activacion: STRING
}




CLASS alarma_acum RESTRICTS alarma
{
   codigo = c / head(c, 3) = "AC_"
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

CLASS error_bd
{
	nom_clase: STRING
	cod: INTEGER
	oper: STRING
}






RULESET LLAMADAS

;-------------------------------------------------------------------------------
;
;	REGLAS PARA LA CREACION DE ALARMAS
;
;-------------------------------------------------------------------------------

; Agrupamos las llamadas e implicamos las alarmas. Cuando descienda el
; numero de llamadas desapareceran solas las alarmas

RULE alarma_acum NORMAL TIMED 3600
{
	lls: {llamada(miga m,
					  fallo f,
					  tecnologia t,
					  servicio_origen so,
					  segmento_a sa)}
	UNTIMED c: categoria_acum(fallo f,
									  tecnologia t,
									  segmento sa)
	/
	(c.tercera > 0) &
	(c.tercera <= count(lls))
	->
;	ca: control_alarma(alarma BD_Damesecuencia(ca.alarma, "SEC_ALARMA"),
;							 llamadas count(lls))
	ca: control_alarma(alarma strtonum("15"),
							 llamadas count(lls))
	alarma(secuencia ca.alarma,
			 servicio so,
			 segmento sa,
			 miga m,
			 codigo sprintf("AC_%s_%s", c.tecnologia, c.fallo),
			 informacion c.fallo)
}

RULE alarma_dens NORMAL TIMED 600
{
	lls: {llamada(miga m,
					  fallo f,
					  tecnologia t,
					  servicio_origen so,
					  segmento_a sa)}
	UNTIMED c: categoria_dens(fallo f,
									  tecnologia t,
									  segmento sa)
	/
	(c.tercera > 0) &
	(c.tercera <= count(lls))
	->
;	ca: control_alarma(alarma BD_Damesecuencia(ca.alarma, "SEC_ALARMA"),
;							 llamadas count(lls))
	ca: control_alarma(alarma strtonum("15"),
							 llamadas count(lls))
	alarma(secuencia ca.alarma,
			 servicio so,
			 segmento sa,
			 miga m,
			 codigo sprintf("DS_%s_%s", c.tecnologia, c.fallo),
			 informacion c.fallo)
}


;-------------------------------------------------------------------------------
;
;	REGLAS PARA EL CAMBIO DE GRAVEDAD DE LAS ALARMAS
;
;-------------------------------------------------------------------------------

; Reglas "autoexplicativas"

RULE asignar_gravedad_3_acum NORMAL
{
	a: alarma_acum(secuencia sec,
						segmento seg,
						informacion i)
	ca: categoria_acum(fallo i,
							 segmento seg)
	co: control_alarma(alarma sec)
	/
	(co.llamadas < ca.segunda) &
	(co.llamadas >= ca.tercera)
	->
;	MODIFY a(gravedad '3',
;				fecha_activacion TM_TextoTiempo(TM_Tiempo()))
	MODIFY a(gravedad '3',
				fecha_activacion sprintf("Gravedad acum 3"))
}

RULE asignar_gravedad_2_acum NORMAL
{
	a: alarma_acum(secuencia sec,
						segmento seg,
						informacion i)
	ca: categoria_acum(fallo i,
							 segmento seg)
	co: control_alarma(alarma sec)
	/
	(co.llamadas < ca.primera) &
	(co.llamadas >= ca.segunda)
	->
;	MODIFY a(gravedad '2',
;				fecha_activacion TM_TextoTiempo(TM_Tiempo()))
	MODIFY a(gravedad '2',
				fecha_activacion sprintf("Gravedad acum 2"))
}

RULE asignar_gravedad_1_acum NORMAL
{
	a: alarma_acum(secuencia sec,
						segmento seg,
						informacion i)
	ca: categoria_acum(fallo i,
							 segmento seg)
	co: control_alarma(alarma sec)
	/
	(co.llamadas >= ca.primera) 
	->
;	MODIFY a(gravedad '1',
;				fecha_activacion TM_TextoTiempo(TM_Tiempo()))
	MODIFY a(gravedad '1',
				fecha_activacion sprintf("Gravedad acum 1"))
}

RULE asignar_gravedad_3_dens NORMAL
{
	a: alarma_dens(secuencia sec,
						segmento seg,
						informacion i)
	ca: categoria_dens(fallo i,
							 segmento seg)
	co: control_alarma(alarma sec)
	/
	(co.llamadas < ca.segunda) &
	(co.llamadas >= ca.tercera)
	->
;	MODIFY a(gravedad '3',
;				fecha_activacion TM_TextoTiempo(TM_Tiempo()))
	MODIFY a(gravedad '3',
				fecha_activacion sprintf("Gravedad dens 3"))
}

RULE asignar_gravedad_2_dens NORMAL
{
	a: alarma_dens(secuencia sec,
						segmento seg,
						informacion i)
	ca: categoria_dens(fallo i,
							 segmento seg)
	co: control_alarma(alarma sec)
	/
	(co.llamadas < ca.primera) &
	(co.llamadas >= ca.segunda)
	->
;	MODIFY a(gravedad '2',
;				fecha_activacion TM_TextoTiempo(TM_Tiempo()))
	MODIFY a(gravedad '2',
				fecha_activacion sprintf("Gravedad dens 2"))
}

RULE asignar_gravedad_1_dens NORMAL
{
	a: alarma_dens(secuencia sec,
						segmento seg,
						informacion i)
	ca: categoria_dens(fallo i,
							 segmento seg)
	co: control_alarma(alarma sec)
	/
	(co.llamadas >= ca.primera) 
	->
;	MODIFY a(gravedad '1',
;				fecha_activacion TM_TextoTiempo(TM_Tiempo()))
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
;	CREATE error_bd(nom_clase "alarma",
;						 oper "insertar",
;						 cod BD_InsertarObjeto(a)) ;
	CREATE error_bd(nom_clase "alarma",
						 oper "insertar",
						 cod strtonum("1"))
	MODIFY c(max_gravedad a.gravedad)
;	CALL ON INSERT EFD_Propagar(a)
	CALL ON INSERT printf("Propago creacion de alarma con fecha %s\n", a.fecha_activacion)
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
;	CREATE error_bd(nom_clase "alarma",
;						 oper "modificar",
;						 cod BD_ModificarObjetos(a,
;							  sprintf("SECUENCIA='%ld'", a.secuencia)))
	CREATE error_bd(nom_clase "alarma",
						 oper "modificar",
						 cod strtonum("2"))
	MODIFY c(max_gravedad a.gravedad)
;	CALL ON INSERT EFD_Propagar(a)
	CALL ON INSERT printf("Propago modificacion de alarma con fecha %s\n", a.fecha_activacion)
}



;-------------------------------------------------------------------------------
;
;	REGLAS PARA ASOCIACION DE LLAMADAS Y ALARMAS
;
;-------------------------------------------------------------------------------

; Estas reglas se disparan en el momento de creacion de una alarma.
; Sirve para asociar la alarma a todas las llamadas que la han provocado.

RULE asignar_alarma_acum_a_llamada NORMAL TIMED 3600
{
	ll: llamada(miga m,
					fallo f,
					servicio_origen so,
					segmento_a sa)
	TRIGGER a: alarma_acum(miga m,
								  servicio so,
								  segmento sa,
								  informacion f) / (a.gravedad != '\0')
	->
	CHANGE ll(sec_alarma_acum a.secuencia)
;	CREATE error_bd(nom_clase "llamada",
;						 oper "modificar",
;						 cod BD_ModificarObjetos(ll,
;							  sprintf("SECUENCIA=%ld", ll.secuencia))) ;
	CREATE error_bd(nom_clase "llamada",
						 oper "modificar",
						 cod strtonum("3"))
;   CREATE error_bd(nom_clase "cliente_afectado",
;                   oper "insertar",
;                   cod strtonum(BD_EjecProc("sgsc_cli.inserta_afectado","N",1,
;                       sprintf("%i,'%s'", a.secuencia, ll.numero_a))))
   CREATE error_bd(nom_clase "cliente_afectado",
                   oper "insertar",
                   cod strtonum("4"))

}

RULE asignar_alarma_dens_a_llamada NORMAL TIMED 600
{
	ll: llamada(miga m,
					fallo f,
					servicio_origen so,
					segmento_a sa)
	TRIGGER a: alarma_dens(miga m,
								  servicio so,
								  segmento sa,
								  informacion f) / (a.gravedad != '\0')
	->
	CHANGE ll(sec_alarma_dens a.secuencia)
;	CREATE error_bd(nom_clase "llamada",
;						 oper "modificar",
;						 cod BD_ModificarObjetos(ll,
;							  sprintf("SECUENCIA=%ld", ll.secuencia))) ;
	CREATE error_bd(nom_clase "llamada",
						 oper "modificar",
						 cod strtonum("5"))
;   CREATE error_bd(nom_clase "cliente_afectado",
;                   oper "insertar",
;                   cod strtonum(BD_EjecProc("sgsc_cli.inserta_afectado","N",1,
;                       sprintf("%i,'%s'", a.secuencia, ll.numero_a))))
   CREATE error_bd(nom_clase "cliente_afectado",
                   oper "insertar",
                   cod strtonum("6"))
}

; Estas reglas se disparan cuando llega una nueva llamada.
; sirve para asociar alarmas que YA existen a nuevas llamadas.

RULE asignar_llamada_a_alarma_acum NORMAL 
{
	TRIGGER ll: llamada(miga m,
							  fallo f,
							  servicio_origen so,
							  segmento_a sa)
	a: alarma_acum(miga m,
						servicio so,
						segmento sa,
						informacion f)
	->
	CHANGE ll(sec_alarma_acum a.secuencia)
;	CREATE error_bd(nom_clase "llamada",
;						 oper "modificar",
;						 cod BD_ModificarObjetos(ll,
;							  sprintf("SECUENCIA=%ld", ll.secuencia))) ;
	CREATE error_bd(nom_clase "llamada",
						 oper "modificar",
						 cod strtonum("7"))
;   CREATE error_bd(nom_clase "cliente_afectado",
;                   oper "insertar",
;                   cod strtonum(BD_EjecProc("sgsc_cli.inserta_afectado","N",1,
;                       sprintf("%i,'%s'", a.secuencia, ll.numero_a))))
   CREATE error_bd(nom_clase "cliente_afectado",
                   oper "insertar",
                   cod strtonum("8"))
}

RULE asignar_llamada_a_alarma_dens NORMAL 
{
	TRIGGER ll: llamada(miga m,
							  fallo f,
							  servicio_origen so,
							  segmento_a sa)
	a: alarma_dens(miga m,
						servicio so,
						segmento sa,
						informacion f)
	->
	CHANGE ll(sec_alarma_dens a.secuencia)
;	CREATE error_bd(nom_clase "llamada",
;						 oper "modificar",
;						 cod BD_ModificarObjetos(ll,
;							  sprintf("SECUENCIA=%ld", ll.secuencia))) ;
	CREATE error_bd(nom_clase "llamada",
						 oper "modificar",
						 cod strtonum("9"))
;   CREATE error_bd(nom_clase "cliente_afectado",
;                   oper "insertar",
;                   cod strtonum(BD_EjecProc("sgsc_cli.inserta_afectado","N",1,
;                       sprintf("%i,'%s'", a.secuencia, ll.numero_a))))
   CREATE error_bd(nom_clase "cliente_afectado",
                   oper "insertar",
                   cod strtonum("10"))
}

END
END

