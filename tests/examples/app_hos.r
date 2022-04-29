PACKAGE foo

CLASS temp
{
  id: INTEGER
}

CLASS evento
{
   id: INTEGER
}

CLASS z
{
   id: INTEGER
}

RULESET bar

RULE A  NORMAL TIMED
{
   te:temp(id x)
   ev:evento(id x)
   ->
   z()
   delete ev
   MODIFY te(id 8)
}

END
END
