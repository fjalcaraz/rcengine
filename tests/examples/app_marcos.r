PACKAGE foo

CLASS al
{
  fecha: INTEGER
  ruta:  STRING
}

CLASS temp
{
   marca: INTEGER
}

CLASS perm
{
  fecha: INTEGER
  ruta:  STRING
}

RULESET bar

RULE A  NORMAL
{
   temp(marca T1)
   al(fecha T2, ruta NR)
   / T1>=T2+240 & strtonum(NR) = strtonum("30N")
   ->
   perm(fecha T2, ruta NR)
}

END
END
