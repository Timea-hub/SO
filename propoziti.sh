#!/bin/bash

if [ $# -ne 1 ];
then
    echo "Scriptul accepta un sigur argument, representat de un caracter"
    exit 1
fi



ch=$1


function count_corect_sentances {
    must_have="^[A-Z][a-zA-Z0-9, ]*[!\?\.]$";
    must_not_have=",[ ]*si[ ]+";

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
        echo $line | grep $ch
    done | wc -l
}

function accumulate_corect_sentances {
    acc=0;
    while read line;
    do
        (( acc += $line ))
    done

    echo "Au fost identificate in total $acc propozitii corecte care contin caracterul $ch"
}