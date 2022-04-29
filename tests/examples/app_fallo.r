PACKAGE fallo

TEMPORAL CLASS llamada
{
	miga:	STRING
	fallo: STRING
	servicio_origen: STRING
	segmento_a: INTEGER
	sec_alarma_acum: INTEGER
	sec_alarma_dens: INTEGER
	secuencia: INTEGER
	numero_a: STRING
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
}

CLASS error_bd
{
	nom_clase: STRING
	cod: INTEGER
	oper: STRING
}

CLASS cliente_afectado
{
	secuencia: INTEGER
	numero_inicial: STRING
}


RULESET fallo

RULE alarma_acum NORMAL TIMED 3600
{
   lls: {llamada(miga m,
                 fallo f,
                 servicio_origen so,
                 segmento_a sa)}
   UNTIMED c: categoria_acum(fallo f,
                             segmento sa)
   /
   (c.tercera > 0) &
   (c.tercera <= count(lls))
   ->
;   a: alarma(secuencia BD_Damesecuencia(a.secuencia, "SEC_ALARMA"),
   a: alarma(secuencia 1,
             servicio so,
             segmento sa,
             miga m,
             codigo sprintf("AC_%s_%s", c.tecnologia, c.fallo),
             informacion c.fallo)
   control_alarma(alarma a.secuencia,
                  llamadas count(lls))
}


RULE alarma_dens NORMAL TIMED 600
{
   lls: {llamada(miga m,
                 fallo f,
                 servicio_origen so,
                 segmento_a sa)}
   UNTIMED c: categoria_dens(fallo f,
                             segmento sa)
   /
   (c.tercera > 0) &
   (c.tercera <= count(lls))
   ->
;   a: alarma(secuencia BD_Damesecuencia(a.secuencia, "SEC_ALARMA"),
   a: alarma(secuencia 2,
             servicio so,
             segmento sa,
             miga m,
             codigo sprintf("DS_%s_%s", c.tecnologia, c.fallo),
             informacion c.fallo)
   control_alarma(alarma a.secuencia,
                  llamadas count(lls))
}


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
   MODIFY a(gravedad '3',
;            fecha_activacion TM_TextoTiempo(TM_Tiempo()))
            fecha_activacion "HOY")
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
   MODIFY a(gravedad '3',
;            fecha_activacion TM_TextoTiempo(TM_Tiempo()))
            fecha_activacion "HOY")
}


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
   ;MODIFY ll(sec_alarma_acum a.secuencia)
   CREATE error_bd(nom_clase "llamada",
                   oper "modificar",
;                   cod BD_ModificarObjetos(ll,
;                       sprintf("SECUENCIA=%ld", ll.secuencia))) ;
                   cod 150)
   CREATE c:cliente_afectado(secuencia a.secuencia,
                             numero_inicial ll.numero_a)
   CREATE error_bd(nom_clase "cliente_afectado",
                   oper "insertar",
                   cod 135)
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
   ;MODIFY ll(sec_alarma_dens a.secuencia)
   CREATE error_bd(nom_clase "llamada",
                   oper "modificar",
;                   cod BD_ModificarObjetos(ll,
;                       sprintf("SECUENCIA=%ld", ll.secuencia))) ;
                   cod 196)
   CREATE c:cliente_afectado(secuencia a.secuencia,
                             numero_inicial ll.numero_a)
   CREATE error_bd(nom_clase "cliente_afectado",
                   oper "insertar",
;                   cod BD_InsertarObjeto(c))
                   cod 548)
}


RULE asignar_llamada_a_alarma_acum NORMAL TIMED 3600
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
   ;MODIFY ll(sec_alarma_acum a.secuencia)
   CREATE error_bd(nom_clase "llamada",
                   oper "modificar",
;                   cod BD_ModificarObjetos(ll,
;                       sprintf("SECUENCIA=%ld", ll.secuencia))) ;
							cod 3434)
   CREATE c:cliente_afectado(secuencia a.secuencia,
                             numero_inicial ll.numero_a)
   CREATE error_bd(nom_clase "cliente_afectado",
                   oper "insertar",
;                   cod BD_InsertarObjeto(c))
                   cod 5478)
}

RULE asignar_llamada_a_alarma_dens NORMAL TIMED 600
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
   ;MODIFY ll(sec_alarma_dens a.secuencia)
   CREATE error_bd(nom_clase "llamada",
                   oper "modificar",
;                   cod BD_ModificarObjetos(ll,
;                       sprintf("SECUENCIA=%ld", ll.secuencia))) ;
							cod 2344)
   CREATE c:cliente_afectado(secuencia a.secuencia,
                             numero_inicial ll.numero_a)
   CREATE error_bd(nom_clase "cliente_afectado",
                   oper "insertar",
;                   cod BD_InsertarObjeto(c))
                   cod 43948)
}


END

END
