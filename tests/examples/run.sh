export LD_LIBRARY_PATH=../../lib
for i in *.i*; do echo $i ;../../bin/tester -i $i ${i%.i*}.r 2>&1 ; done
