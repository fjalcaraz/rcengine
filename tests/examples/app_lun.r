PACKAGE fallo

TEMPORAL CLASS ll
{
        miga:   INTEGER
}

CLASS ala
{
        miga: INTEGER
        sec: INTEGER
        grav: INTEGER
}


CLASS cat
{
       miga: INTEGER
       seg:  INTEGER
}


CLASS ctrl
{
       llam: INTEGER
       sec: INTEGER
}

RULESET fallo

RULE a NORMAL TIMED 600
{ 
   lls:{ll(miga m) } 
   /
   count (lls) >=1
   ->
   a:ala (miga m, sec 2, grav 0)
   ctrl(sec  2, llam count(lls))
}

RULE b NORMAL
{
  a:ala(miga m, sec s)
  ca:cat(miga m)
  c:ctrl(sec s)
  /
  c.llam < ca.seg
  ->
  CALL printf("HELLO\n")
}

END 
END
