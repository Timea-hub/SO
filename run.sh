#!/bin/bash

rm -rf input_dir output_dir
mkdir output_dir

cp -R source_dir input_dir

cd input_dir

mkdir inner_dir
ln -s opie.bmp s_opie
ln -s inner_dir s_innner


cd ..

gcc -Wall -o week9 week9.c
./week9 input_dir output_dir a