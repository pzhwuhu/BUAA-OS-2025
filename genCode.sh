#! /bin/bash

mkdir codeSet
for file in $(ls code/)
do
	cp $file codeSet/
done

for file in $(ls codSet)
do
	sed -i '1i#include"include/libsy.h"' file
	sed -i 's/getInt/getint/g' file
	mv $file $file.c
done
