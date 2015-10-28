#! /bin/bash

MIN_SIZE=1
MAX_SIZE=15
STEP=1
ITER=10

for i in `seq $MIN_SIZE $STEP $MAX_SIZE`; do
#    echo "=== # CPU = $i ==="
    echo $i
    python genmat.py $i
    for j in `seq $ITER`; do
#        ./test.sh
        np=$(( $i * $i ))
        mpirun --prefix /usr/local/share/OpenMPI -np $np mm
    done
done

