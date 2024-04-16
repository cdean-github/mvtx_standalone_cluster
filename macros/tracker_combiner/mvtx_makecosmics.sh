#!/usr/bin/bash

# creates file lists for the MVTX from known locations in lustre
# run number is the input argument

if [ $# -eq 0 ]
  then
    echo "Creates needed lists of input files for the MVTX for a given run"
    echo "Usage: mvtx_makelist.sh <run number>"
    exit 1
fi

runnumber=$(printf "%08d" $1)

for i in {0..5}
do
ls -1 /sphenix/lustre01/sphnxpro/commissioning/MVTX/cosmics/cosmics_mvtx${i}-${runnumber}-* > tracker_combiner/input_lists/mvtx${i}_${runnumber}.list
if [ ! -s tracker_combiner/input_lists/mvtx${i}_${runnumber}.list ]
then
  echo tracker_combiner/input_lists/mvtxx${i}_${runnumber}.list empty, removing it
  rm tracker_combinerinput_lists/mvtx${i}_${runnumber}.list
fi

done
