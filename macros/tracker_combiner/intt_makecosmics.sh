#!/usr/bin/bash

# creates file lists for the INTT from known locations in lustre
# run number is the input argument

if [ $# -eq 0 ]
  then
    echo "Creates needed lists of input files for the Intt for a given run"
    echo "Usage: intt_makelist.sh <run number>"
    exit 1
fi

runnumber=$(printf "%08d" $1)

for i in {0..7}
do
/bin/ls -1 /sphenix/lustre01/sphnxpro/commissioning/INTT/cosmics/cosmics_intt${i}-${runnumber}-* > tracker_combiner/input_lists/intt${i}_${runnumber}.list
if [ ! -s tracker_combiner/input_lists/intt${i}_${runnumber}.list ]
then
  echo tracker_combiner/input_lists/intt${i}_${runnumber}.list empty, removing it
  rm tracker_combiner/input_lists/intt${i}_${runnumber}.list
fi

done
