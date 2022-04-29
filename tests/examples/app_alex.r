PACKAGE foo

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


CLASS al
{
   secuencia   : INTEGER
   contador    : INTEGER
   miga        : STRING
}


PROCEDURE genera_llamadas(INTEGER, INTEGER, INTEGER)

CLASS d  IS_A al {}

RULESET bar

RULE B NORMAL
{
   g:genera()
->
   CALL ON INSERT genera_llamadas(g.nllamadas, g.tiempo, g.delta)
}

RULE C LOW  TIMED 1080
{
   ll:llamada(miga x)
   UNTIMED al:al(miga x)
->
   MODIFY ll(secuencia al.secuencia)
}

RULE A NORMAL  TIMED 1080
{
   conj:{llamada(miga x)} / count(conj)>20
->
   al:al(secuencia 1, miga x, contador count(conj))
   call printf("time = %d\n", time(2))
}

END
END
