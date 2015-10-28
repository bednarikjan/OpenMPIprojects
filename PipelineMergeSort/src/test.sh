#!/bin/bash

numbers=$1
CPUs=$(echo "$(echo "l($numbers) / l(2) + 1" | bc -l)/1" | bc)

#preklad cpp zdrojaku
mpic++ -std=c++11 --prefix /usr/local/share/OpenMPI -o pms pms.cpp

#vyrobeni souboru s random cisly
dd if=/dev/urandom bs=1 count=$numbers of=numbers

#spusteni
mpirun --prefix /usr/local/share/OpenMPI -np $CPUs pms

#uklid
rm -f numbers pms
