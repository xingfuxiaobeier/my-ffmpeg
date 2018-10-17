#!/bin/bash
st=""
count=0
for i in `cat ./test.log|grep "pkt_dts="|awk -F "=" '{print $2}'`
do
#	echo $ii
	if [ -n $i ];then
#		echo $i
		st=${st}${i}","
		((count=${count}+1))
	fi
#	st=${st}${i}","
done
st="{"${st}"}"
echo "count="${count}
echo $st

	 	

