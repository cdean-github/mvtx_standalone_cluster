#!/usr/bin/bash

if [ $# -eq 0 ]
then
  echo "No runnumber supplied"
  exit 0
fi

runnumber=$(printf "%08d" $1)
/bin/ls -1 /sphenix/lustre01/sphnxpro/commissioning/TPOT/cosmics/TPOT_ebdc39_cosmics-${runnumber}-* > tracker_combiner/input_lists/tpot_${runnumber}.list
if [ ! -s tracker_combiner/input_lists/tpot_${runnumber}.list ]
then
  echo tracker_combiner/input_lists/tpot_${runnumber}.list empty, removing it
  rm  tracker_combiner/input_lists/tpot_${runnumber}.list
fi
