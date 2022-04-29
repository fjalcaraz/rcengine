PACKAGE Conj1

class A
{
  cid: INTEGER
  valor: INTEGER
  str: STRING
}

class FUNCIONES_CONJ
{
  mini: INTEGER
  maxi: INTEGER
  suma: INTEGER
  producto: INTEGER
  cardinal: INTEGER
  concatenado: STRING
}

RULESET RSConj1

RULE uno NORMAL
{
  conja:{ a:A(cid cida) / a.valor > 0 }
->
  f:FUNCIONES_CONJ(mini min(conja.valor),
                 maxi max(conja.valor),
                 suma sum(conja.valor),
                 producto prod(conja.valor),
                 cardinal count(conja),
                 concatenado concat(conja.str,"|")
                 )

  CALL printf("Conjunto identificador %d:\nMinimo: %d\nMaximo: %d\nSuma: %d\nProducto: %d\nCardinal: %d\nCadena: %s\n",
               cida, f.mini, f.maxi, f.suma, f.producto, f.cardinal, f.concatenado);

}

END ;RULESET
END ;PACKAGE
