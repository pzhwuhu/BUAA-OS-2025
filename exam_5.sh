#! /bin/bash

i=0;
while [ $i -le 20 ]
do
	sed -i 's/REPLACE/str/i/g' origin/code/$i.c
	cp origin/code/$i.c result/code/$i.c
	let i=i+1
done
