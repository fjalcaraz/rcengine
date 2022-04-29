PACKAGE foo

CLASS llamada
{
 id                     : INTEGER
}

CLASS alarma 
{
  id             : INTEGER
}

CLASS z 
{
  id             : INTEGER
}

RULESET bar

RULE B NORMAL TIMED 20
{
   ll:llamada()
   al:[alarma()]
->
   z()
}

END
END
