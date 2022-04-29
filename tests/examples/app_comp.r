PACKAGE CASQUE

CLASS  a
{
  id: STRING
}

CLASS  b
{
  id: STRING
  num: INTEGER
}

CLASS  c
{
  id: STRING
}

RULESET INCREIBLE
 
  RULE a NORMAL
  {
    a()
    b:b()
    !c(id x)
/ length(x) = b.num
;    / length(x) = length(b.id)
    ->
    DELETE 1
  }
 
END
END
