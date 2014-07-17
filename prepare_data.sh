#!/bin/bash

echo -e "nr_reads\tnr_writes\tnr_ops" > tmp.txt

awk '{print $16"\t"$18"\t"$20}' $1 >> tmp.txt

mv tmp.txt $1
