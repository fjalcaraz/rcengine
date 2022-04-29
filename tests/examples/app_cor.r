PACKAGE foo

CLASS arranque
{
  valor: BOOLEAN
}

CLASS bk_fecha_inicial
{
  fecha: INTEGER
}

CLASS z
{
  numero: INTEGER
}


RULESET bar

RULE B NORMAL
{
  algo: arranque() / algo.valor   = FALSE
  la_fecha: !bk_fecha_inicial()
->
 CREATE z(numero 10)
}

END
END
