PACKAGE foo

TEMPORAL CLASS ll
{
   miga        : STRING
}

CLASS al
{
   miga        : STRING
}


RULESET bar

RULE C LOW  TIMED 5
{
   ll:ll(miga x)
   al:al(miga x)
->
   MODIFY ll(miga x)
}

END
END
