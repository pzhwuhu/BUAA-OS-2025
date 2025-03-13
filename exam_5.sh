#! /bin/bash

i=0;
sed -i 's/REPLACE/0/g' origin/code/0.c
sed -i 's/REPLACE/1/g' origin/code/1.c
sed -i 's/REPLACE/2/g' origin/code/2.c
sed -i 's/REPLACE/3/g' origin/code/3.c
sed -i 's/REPLACE/4/g' origin/code/4.c
sed -i 's/REPLACE/5/g' origin/code/5.c
sed -i 's/REPLACE/6/g' origin/code/6.c
sed -i 's/REPLACE/7/g' origin/code/7.c
sed -i 's/REPLACE/8/g' origin/code/8.c
sed -i 's/REPLACE/9/g' origin/code/9.c
sed -i 's/REPLACE/10/g' origin/code/10.c
sed -i 's/REPLACE/11/g' origin/code/11.c
sed -i 's/REPLACE/12/g' origin/code/12.c
sed -i 's/REPLACE/13/g' origin/code/13.c
sed -i 's/REPLACE/14/g' origin/code/14.c
sed -i 's/REPLACE/15/g' origin/code/15.c
sed -i 's/REPLACE/16/g' origin/code/16.c
sed -i 's/REPLACE/17/g' origin/code/17.c
sed -i 's/REPLACE/18/g' origin/code/18.c
sed -i 's/REPLACE/19/g' origin/code/19.c
sed -i 's/REPLACE/20/g' origin/code/20.c
while [ $i -le 20 ]
do
	cp origin/code/$i.c result/code/$i.c
	let i=i+1
done
