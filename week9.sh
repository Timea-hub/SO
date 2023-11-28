#!/bin/bash

if [ $# -ne 1 ];
then
    echo "Scriptul accepta un sigur argument, representat de un caracter"
    exit 1
fi

must_have="^[A-Z][a-zA-Z0-9, ]*[!\?\.]$"
must_not_have=",[ ]*si"

while read line;
do
    echo $line | grep "$must_have" > /dev/null
    if [ $? -ne 0 ];
    then 
        continue
    fi
    echo $line | grep "$must_not_have" > /dev/null  
    if [ $? -eq 0 ];
    then 
        continue 
    fi
    echo $line | grep "$1" 
done | wc -l
