#! /bin/bash

i=0;
while(($i <= 20))
do
	sed -i 's/REPLACE/$i/g' "$i.c"
	let i=i+1
done
