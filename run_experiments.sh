#!/bin/bash

OUT_DIR=data
TEST_RCU=./userspace-rcu/tests/benchmark/test_urcu_assign
TEST_RWLOCK=./userspace-rcu/tests/benchmark/test_rwlock

NUM_READER_THREADS=100
DURATION=3

num_writers_options=(10 25 50 75 100)

rm $OUT_DIR/*.txt

# RCU test loop
for num_writers in ${num_writers_options[*]}
do
	for i in {1..100}
	do
		$TEST_RCU $NUM_READER_THREADS $num_writers $DURATION >> $OUT_DIR/rcu-$num_writers.txt
	done
done

# RWLock test loop
for num_writers in ${num_writers_options[*]}
do
	for i in {1..100}
	do
		$TEST_RWLOCK $NUM_READER_THREADS $num_writers $DURATION >> $OUT_DIR/rwlock-$num_writers.txt
	done
done
