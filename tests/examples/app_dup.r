PACKAGE foo

CLASS a
{
 sec                           : INTEGER
 miga                          : STRING
}

CLASS b IS_A a {}

RULESET bar

RULE detecta_duplicados NORMAL
{
   a(classname "a", sec x, miga y)
   a(classname "a", sec z, miga y)  / z>x
->
   delete 2
}


RULE copia LOW
{
  a:a(classname "a")
->
  CREATE b(sec a.sec, miga a.miga)
}


RULE particular NORMAL
{
  b:b(miga "alfa")
->
  CALL printf("TENGO un B sec %d  miga %s!!\n", b.sec, b.miga)
}
end
END
