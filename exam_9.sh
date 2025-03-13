#! /bin/bash

if (($# == 2))
then
	sed '$1,$2' stderr.txt
elif (($# == 1))
then
	sed '$1,$d' stderr.txt
else
then
	cat stderr.txt
fi
