PACKAGE foo

TEMPORAL CLASS ll
{
   secuencia   : INTEGER
   miga        : STRING
}

CLASS al
{
   secuencia   : INTEGER
   contador    : INTEGER
   miga        : STRING
   tipo        : STRING
}


CLASS cat
{
  miga: STRING
  tipo: STRING
  umbral: INTEGER
}

FUNCTION BD_DameSecuencia ( INTEGER, STRING ) = INTEGER


RULESET bar

RULE B NORMAL TIMED 20
{
  UNTIMED cat:cat(miga m, tipo "DS")
  ll:{ ll(miga m)} / count(ll)>=cat.umbral
->
  al:al(contador count(ll), miga m, secuencia BD_Damesecuencia(al.secuencia, "SEC_ALARMA"), tipo cat.tipo)
  CALL ON RETRACT printf("VAMOS, ATRAS ATRAS....\n")
  CALL ON RETRACT destroy_set(ll, 1)
}

;RULE C NORMAL TIMED 120
;{
;  UNTIMED cat:cat(miga m, tipo "AC")
;  TIMER_KEY 2 ll:{ ll(miga m)} / count(ll)>=cat.umbral
;->
;  al:al(contador count(ll), miga m, secuencia BD_Damesecuencia(al.secuencia, "SEC_ALARMA"), tipo cat.tipo)
;}

RULE D NORMAL  TIMED 20
{
   ll:ll(miga x)
   UNTIMED TRIGGER al:al(miga x)
->
   CALL ON INSERT printf("ALARMA QUE LLEGA\n")
   CHANGE ll(secuencia al.secuencia)
}


;RULE E NORMAL
;{
;   TRIGGER ll:ll(miga x)
;   al:al(miga x)  
;->
;   CALL ON INSERT printf("LLAMADA QUE LLEGA DESPUES DE LA ALARMA\n")
;   CHANGE ll(secuencia al.secuencia)
;}

RULE E NORMAL TIMED 20
{
   TEMPORAL ll:ll(miga x)
   UNTIMED al:al(miga x)  
->
   CALL ON INSERT printf("LLAMADA QUE LLEGA DESPUES DE LA ALARMA\n")
   CHANGE ll(secuencia al.secuencia)
}
END
END
