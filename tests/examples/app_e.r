PACKAGE viruta

CLASS error_bd
{
	nom_clase: STRING
	oper: STRING
	texto: STRING
}

CLASS bk_backup
{
        fecha_cese : STRING
        fecha_backup : STRING
}
 
CLASS bk_fecha_inicial
{
        fecha : INTEGER
}
 
CLASS bk_grupo_tablas
{
        nombre  : STRING
}
CLASS bk_estado_backup
{
        estado : STRING
}
 
CLASS tm_evento
{
  nombre                : STRING
}


RULESET REGLAS_BACKUP
; ___________________________________________________________________
; ___________________________________________________________________
RULE bk_comenzar LOW
{
	evento:tm_evento( nombre "ev_backup" )
	->
	CREATE bk_backup(fecha_cese "1", fecha_backup "2")
 	CREATE bk_grupo_tablas(nombre "Hemos inventado el sifon")
	CREATE bk_estado_backup(estado "realizar_backup")
}
RULE bk_realizar_backup LOW
{
	e:bk_estado_backup(estado "realizar_backup")
	b:bk_backup(fecha_cese fc, fecha_backup fb)
 	g:bk_grupo_tablas(nombre n)
	->
	CREATE error_bd(nom_clase "backup", oper "backup", texto "TPMEB")
	MODIFY e(estado "notificar_iu");
}
;RULE bk_notificar_iu LOW
;{
;	e:bk_estado_backup(estado "notificar_iu")
;	b:bk_backup(fecha_cese fc, fecha_backup fb)
;	->
;	MODIFY e(estado "finalizar")
;}
;RULE bk_finalizar LOW
;{
;	e:bk_estado_backup(estado "finalizar")
;	b:bk_backup()
;	g:bk_grupo_tablas()
;	->
;	DELETE b
;	DELETE e
;	DELETE g
;}	
	
END

END
