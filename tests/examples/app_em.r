PACKAGE foo

CLASS e
{
   clv   : STRING
   sa		 : INTEGER
	 estado : INTEGER
   ad		: STRING
   cnt   : INTEGER
   gen_sec_alar :INTEGER
}

CLASS x
{
}

RULESET bar


RULE ignora LOW
{
	a:e(sa 0, estado 0)
	->
	MODIFY a(estado 1)
  CALL ON INSERT printf("Cogemos datos de bd para espontaneo clv %s\n",a.clv)


}

RULE asocia NORMAL 
{
  e:e(sa 0, estado 1)  
  ->
  MODIFY e(cnt 1, sa e.gen_sec_alar)
  CALL ON INSERT printf("Alarma generada sa %d, cnt %d, datos %s\n",e.sa,e.cnt,e.ad)
}

RULE reten  NORMAL
{
   v:e(clv c, sa x, estado 1)
   n:e(clv c, sa 0, estado 0) / v.sa >0 
   ->
	 DELETE n
   MODIFY v(cnt v.cnt+1, ad n.ad)
   CALL ON INSERT printf("Modificada alarma sa %d, cnt %d, datos %s\n",v.sa,v.cnt,v.ad) 
	 
}




END
END
