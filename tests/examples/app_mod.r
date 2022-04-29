PACKAGE foo

;TEMPORAL CLASS llamada
CLASS llamada
{
 miga                          : STRING
 secuencia                     : INTEGER
}

CLASS alarma 
{
  secuencia             : INTEGER
  miga                  : STRING
}

RULESET bar

RULE B NORMAL TIMED 20
{
   ll:llamada()
->
   MODIFY ll(miga "pepe")
}

END
END
