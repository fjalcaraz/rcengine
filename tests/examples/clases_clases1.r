PACKAGE Clases1
; Pruebas de herencia

ABSTRACT CLASS Madre
{
  atribmadre1: INTEGER
  atribmadre2: STRING
}

CLASS Madre2
{
  atribmadre21: STRING
  atribmadre22: INTEGER
}

; Herencia basica de Madre abstracta
CLASS Hija1 IS_A Madre
{
  atribhija11: INTEGER
  atribhija12: INTEGER
}

; Herencia basica de Madre abstracta
CLASS Hija2 IS_A Madre
{
  atribhija21: INTEGER
  atribhija22: INTEGER
}

CLASS Hija3 IS_A Madre2
{
  atribhija31: INTEGER
}

; Restriccion de atributo no heredado
CLASS Prima1 RESTRICTS Hija1
{
  atribhija11 = X / X > 100
}

; Restriccion de atributo heredado
CLASS Prima2 RESTRICTS Hija1
{
  atribmadre1 = X / X > 100
}

; Restriccion de atributo heredado y redefinido
CLASS Prima3 RESTRICTS Hija2
{
  atribmadre2 = "SOY_PRIMA_3"
}


RULESET RClases1

RULE regla1 NORMAL
{
  Hija1(atribmadre1 X)
->
  CALL printf("Herencia base de madre abstracta\nValor del atributo de la madre: %d\n",X)
}

RULE regla2 NORMAL
{
  Hija2(atribmadre2 X)
->
  CALL printf("Herencia con redefinicion\nValor de atributo redefinido: %s\n",X)
}

RULE regla6 NORMAL
{
  Prima1(atribhija11 X)
->
  CALL printf("Restriccion atributo no heredado\nValor del atributo: %d\n",X)
}

RULE regla7 NORMAL
{
  Prima2(atribmadre1 X)
->
  CALL printf("Restriccion atributo heredado\nValor del atributo: %d\n",X)
}

RULE regla8 NORMAL
{
  Prima3(atribmadre2 X)
->
  CALL printf("Restriccion atributo redefinido\nValor del atributo: %s\n",X)
}

END ; RULESET

END ; PACKAGE
