PACKAGE Ejemplo3

CLASS a
{
   a1       : INTEGER
}

CLASS b
{
   b1       : INTEGER
}

CLASS c
{
   c1       : INTEGER
}

CLASS d
{
   d1       : INTEGER
}

RULESET Ejem3RS

RULE Regla1 NORMAL
{
; ANTECEDENTE
a(a1 x)
b(b1 x)
->
; CONSECUENTE
c(c1 x)
}

RULE Regla2 NORMAL
{
a(a1 x)
! c(c1 x)
->
d(d1 x)
}

; FIN RULESET Ejemplos
END

; FIN PAQUETE
END
