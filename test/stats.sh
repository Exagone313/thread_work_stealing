#!/bin/bash
if [ -z "${TEST}" ]; then
	TEST='quicksort'
fi
args='-x 100'
threads=(1 2 4 5 8 9 16 17)
strategies=('lifo' 'wsteal')
echo ${TEST} ${args}
echo serial=$("./${TEST}.serial" ${args})
for s in "${strategies[@]}"; do
	echo ${s}
	for t in "${threads[@]}"; do
		echo ${t},$("./${TEST}.${s}" ${args} -t ${t})
	done
done
