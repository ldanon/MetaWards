#!/bin/bash

COUNT=1
# bash until loop
until [ $COUNT -gt $2 ]; do
    mkdir $COUNT
    cd $COUNT
    nohup ../$1 $RANDOM >/dev/null 
#    sleep 2
    cd ..
    let COUNT=COUNT+1
done
