PACKAGE bolsa

TRIGGER CLASS value
{
    symbol : STRING
    value  : FLOAT
}

CLASS alert
{
    user_id : INTEGER
    media   : STRING
    text    : STRING
}

RULESET reglas

RULE fulanitez NORMAL
{
   value(symbol "BVSN", value v) / v>300.0
   ->
   create alert(user_id 2030, media "SMS", text "Ha subido por encima de 300")
}

RULE menganitez NORMAL
{
   value(symbol "BVSN", value v) / v<90.0
   ->
   create alert(user_id 2030, media "SMS", text "Ha bajado por debajo de 90")
}

END
END
