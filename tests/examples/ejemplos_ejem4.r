PACKAGE Ejemplo4

CLASS a
{
  idconj: INTEGER
  ax: INTEGER
}

CLASS b
{
  idconj: INTEGER
  cardinal: INTEGER
}

RULESET Reglas4

RULE uno NORMAL
{
conj: { obj:a(idconj IDC) / obj.ax > 0 }
->
b(idconj IDC, cardinal count(conj) )
}

RULE dos HIGH
{
objb:b(idconj IDC, cardinal C) / C > 2
conj: { a(idconj IDC) }
->
delete objb
}

; FIN RULESET
END

END
