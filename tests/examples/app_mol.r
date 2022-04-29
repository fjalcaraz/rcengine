PACKAGE foo

CLASS z
{
 miga                          : STRING
 secuencia                     : INTEGER
}


CLASS llamada
{
 miga                          : STRING
 secuencia                     : INTEGER
}

CLASS control 
{
  secuencia             : INTEGER
  miga                  : STRING
  ruta                  : STRING
}

CLASS alarma 
{
  numero_ll             : INTEGER
  miga                  : STRING
  ruta                  : STRING
}

RULESET bar

RULE B NORMAL
{
   lls:{llamada()}
->
   control(secuencia count(lls))
   alarma()
}


RULE C NORMAL
{
   cl:control()
   al:alarma()  / cl.secuencia >1
->
   MODIFY al(ruta "jamon")
}

RULE D LOW
{
alarma()
->
z()
}


END
END
