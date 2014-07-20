#!/bin/bash

echo -e "nr_reads\tnr_writes\tnr_ops" > tmp.txt

if [ "$2" == "kernel" ]
then
    awk '{print substr($6, 0, length($6)-1)"\t"$8"\t"substr($6, 0, length($6)-1) + $8}' $1 > tmp.txt
else
    awk '{print $16"\t"$18"\t"$20}' $1 >> tmp.txt
fi

mv tmp.txt $1
