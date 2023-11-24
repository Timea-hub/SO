#!/bin/bash

if ["#" - ne 1]; then
    echo "Utilizare: $0 caracter"
    exit 1
fi

k=0

while IFS = read -r line;

do
    if [[$line =~ [A-Z][A-Za-z0-9,\.!\?]$ && ! $line =~ ,*[si]$]]; then

    ((k++))

    fi
done

echo $k
