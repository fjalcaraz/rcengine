PACKAGE Pri1

CLASS a
{
}

RULESET RSPri1

RULE A LOW
{
a()
->
CALL printf("Disparada A\n");
}

RULE B NORMAL
{
a()
->
CALL printf("Disparada B\n");
}

RULE C HIGH
{
a()
->
CALL printf("Disparada C\n");
}

RULE D HIGH
{
a()
->
CALL printf("Disparada D\n");
}

RULE E NORMAL
{
a()
->
CALL printf("Disparada E\n");
}

RULE F LOW
{
a()
->
CALL printf("Disparada F\n");
}

END ;RULESET

END ;PACKAGE
