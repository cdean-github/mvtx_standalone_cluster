#!/usr/bin/bash

if [ $# -eq 0 ]
then
  echo "No runnumber supplied"
  exit 0
fi

runnumber=$(printf "%08d" $1)

for i in {0..23}
do
ebdc=$(printf "%02d" $i)
/bin/ls -1 /sphenix/lustre01/sphnxpro/commissioning/tpc/cosmics/TPC_ebdc${ebdc}_cosmics-${runnumber}-* > tracker_combiner/input_lists/tpc${ebdc}_${runnumber}.list
if [ ! -s tracker_combiner/input_lists/tpc${ebdc}_${runnumber}.list ]
then
  echo tracker_combiner/input_lists/tpc${ebdc}_${runnumber}.list empty, removing it
  rm  tracker_combiner/input_lists/tpc${ebdc}_${runnumber}.list
fi
done
