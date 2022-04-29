PACKAGE Tempo1

TEMPORAL CLASS a
{
}

TEMPORAL CLASS b
{
  comun: INTEGER
}

RULESET RSTempo1

RULE A NORMAL TIMED 3
{
  a()
->
  CALL ON INSERT printf("Entrada de objeto a\n");
  CALL ON RETRACT printf("Caducidad de objeto a\n"); 
}

RULE B NORMAL TIMED 3
{
  b(comun X)
->
  CALL ON INSERT printf("Entrada de objeto b, %d\n",X);
  CALL ON RETRACT printf("Caducidad de objeto b, %d\n",X);
}

END ;RULESET

END ;PACKAGE
