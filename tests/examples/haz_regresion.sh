#!/bin/sh

for package in `ls *.r`
do
	bname=`basename $package .r`
	facts=${bname}.i
#	output=${bname}.out
	echo "## PROBANDO PAQUETE $package"
	regresion -i $facts $package
done
