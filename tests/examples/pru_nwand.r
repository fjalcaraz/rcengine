PACKAGE foo

TEMPORAL CLASS ll
{
   miga        : STRING
}

CLASS al
{
   miga        : STRING
}

CLASS imp
{
	   miga        : STRING
}

RULESET bar

RULE C LOW  TIMED 5
{
   ll:ll(miga x)
   !al(miga x)
->
   imp(miga x)
}

END
END
