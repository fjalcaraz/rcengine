PACKAGE bolsa

TRIGGER CLASS value
{
    symbol : STRING
    value  : FLOAT
}

UNTIMED CLASS alert_setting
{
    user_id : INTEGER
    media   : STRING
    symbol  : STRING
    compar  : INTEGER
    value   : FLOAT
    text    : STRING
}

CLASS alert_tentative
{
    user_id : INTEGER
    type    : INTEGER
    media   : STRING
    text    : STRING
}

CLASS alert
{
    user_id : INTEGER
    media   : STRING
    text    : STRING
}

RULESET reglas

RULE menor_que NORMAL
{
   alert_setting(user_id u, media m, symbol s, compar -1, value vl, text t)
   value(symbol s, value v) / v < vl
->
   CREATE alert_tentative(type 1, user_id u, media m, text sprintf(t, v))
}
   
RULE igual NORMAL
{
   alert_setting(user_id u, media m, symbol s, compar 0, value vl, text t)
   value(symbol s, value v) / v = vl
->
   CREATE alert_tentative(type 0, user_id u, media m, text sprintf(t, v))
}
   
RULE mayor_que NORMAL
{
   alert_setting(user_id u, media m, symbol s, compar 1, value vl, text t)
   value(symbol s, value v) / v > vl
->
   CREATE alert_tentative(type -1, user_id u, media m, text sprintf(t, v))
}
   
RULE contencion_TIMED HIGH TIMED 60
{
   a: alert_tentative(user_id u)
   b: alert_tentative(user_id u) / time(a) > time(b)
->
   delete a
}

RULE contencion_FUERA NORMAL
{
   a: alert_tentative(user_id u)
   b: alert_tentative(user_id u) / time(a) > time(b)
->
   delete b
}


RULE execute LOW
{
   alert_tentative(user_id u, media m, text t)
->
   CREATE alert(user_id u, media m, text t)
   CALL printf("ALERTA *************************\n");
}
END
END
