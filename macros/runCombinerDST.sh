#!/bin/bash

if [ $# -eq 0 ]
  then
    echo "No arguments supplied."
    echo "You must at least specify a run number."
    echo "The second argument is the number of events. If no argument is set, all events will be processed."
    exit 1
fi

source /opt/sphenix/core/bin/sphenix_setup.sh -n new

#export HOME=/sphenix/u/${LOGNAME}
export HOME=/sphenix/u/cdean
export SPHENIX=$HOME/sPHENIX
export MYINSTALL=$SPHENIX/install
export LD_LIBRARY_PATH=$MYINSTALL/lib:$LD_LIBRARY_PATH
export ROOT_INCLUDE_PATH=$MYINSTALL/include:$ROOT_INCLUDE_PATH

fileFinderPath=tracker_combiner
inputListPath=${fileFinderPath}/input_lists

source /opt/sphenix/core/bin/setup_local.sh $MYINSTALL

nEvents=0
if [[ "$2" != "" ]]; then
    nEvents="$2"
fi

echo running: runCombiner.sh
root.exe -q -b Fun4All_Silicon_Analyser.C\($nEvents,\"$1\"\)
echo Script done
