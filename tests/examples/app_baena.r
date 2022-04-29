PACKAGE paquete
CLASS Evento
{
        tipo: INTEGER
        status: INTEGER
        objeto: STRING
}
CLASS Alarma {
        id: INTEGER
        tipo: INTEGER
        categoria: INTEGER
        status: INTEGER
        objeto: STRING

}
RULESET rs

RULE test NORMAL TIMED
{
        o:Evento(tipo t) /t=5
->
        CREATE Alarma(tipo o.tipo, objeto "")
        CREATE Evento(tipo t, status o.status, objeto "")
        CREATE Alarma(tipo t, objeto numtostr(t))
}

END
END
