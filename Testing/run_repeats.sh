#!/bin/bash

# $1 is the wards.o executable
# $2 is the parameters file name
# $3 is the starting lines in the parameters file name
# $COUNT is the line to be read from parameters file 
ZERO=0
COUNT=$3
# echo $(($3+3))
# bash until loop
until [ $COUNT -gt $(($3+100)) ]; do
    echo $COUNT
    mkdir $COUNT
    cd $COUNT
#    nohup ../$1 $RANDOM ../$2 $COUNT> /dev/null 
    echo $(pwd)
    echo nohup ../$1 $RANDOM ../$2 $COUNT> /dev/null 
    nohup ../$1 $RANDOM ../$2 $COUNT> /dev/null 
#    sleep 2
    cd ..
    let COUNT=COUNT+1
done
