#!/bin/bash
set -eu
FILES=./testRecipes/*.E

for f in $FILES
do
	rm -f NodeTable.txt EdgeTable.txt TESToutput.txt SLiM_run_output.log

    echo "Now testing SLiM Recipe: $f"
 	../../bin/slim -s 21 $f &> SLiM_run_output.log
	python3 -m nose test_ancestral_marks.py
done
