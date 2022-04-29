PACKAGE Ejemplo1

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

RULESET Ejemplos

RULE primera NORMAL
{
; ANTECEDENTE
a(a1 x)
b(b1 x)
->
; CONSECUENTE
c(c1 x)
}

; FIN RULESET Ejemplos
END

; FIN PAQUETE
END
