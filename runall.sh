#!/bin/bash
NUM=100000

function run {
    echo $1 $NUM
    ./bench --inserts $NUM --selects $NUM
}

function runor {
    echo $1 OR $NUM
    ./bench --inserts $NUM --selects $NUM --use-or
}

make bench &>/dev/null

make gin &>/dev/null
run GIN

make gin &>/dev/null
runor GIN


make gist &>/dev/null
run GIST

make gist &>/dev/null
runor GIST
