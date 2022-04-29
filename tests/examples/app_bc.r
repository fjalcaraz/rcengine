PACKAGE foo

CLASS cat
{
   id : INTEGER
}

RULESET bar

RULE D  NORMAL TIMED 100
{
   cat(id i)
   /
TRUE
   ->
    cat(id i+1)
}

END
END
