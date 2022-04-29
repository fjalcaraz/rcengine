PACKAGE foo

TRIGGER CLASS trg
{
miga: STRING
}


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

RULE B NORMAL
{
   trg()
   ll:llamada()
->
   MODIFY ll(secuencia ll.secuencia+1)
   ;alarma(secuencia ll.secuencia)
}


RULE C NORMAL
{
   al:alarma()
   ll:llamada() / ll.secuencia <2 & ll.secuencia >=1
->
   MODIFY al(miga sprintf("rulec"))
}



RULE D NORMAL
{
   al:alarma()
   ll:llamada() / ll.secuencia <3 & ll.secuencia >=2
->
   MODIFY al(miga sprintf("ruled"))
}

RULE E NORMAL
{
   al:alarma()
   ll:llamada() / ll.secuencia >=3 
->
   MODIFY al(miga sprintf("rulee"))
}
END
END
