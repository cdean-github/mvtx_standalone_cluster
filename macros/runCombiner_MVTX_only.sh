#!/bin/bash

if [ $# -eq 0 ]
  then
    echo "No arguments supplied."
    echo "You must at least specify a run number."
    echo "The second argument is the number of events. If no argument is set, all events will be processed."
    exit 1
fi


source /opt/sphenix/core/bin/sphenix_setup.sh -n new.9

#export HOME=/sphenix/u/${LOGNAME}
export HOME=/sphenix/u/cdean
export SPHENIX=$HOME/sPHENIX
export MYINSTALL=$SPHENIX/install
export LD_LIBRARY_PATH=$MYINSTALL/lib:$LD_LIBRARY_PATH
export ROOT_INCLUDE_PATH=$MYINSTALL/include:$ROOT_INCLUDE_PATH

source /opt/sphenix/core/bin/setup_local.sh $MYINSTALL

fileFinderPath=mvtx_standalone
inputListPath=${fileFinderPath}/input_lists

simpleRunNumber=$1
runNumber=$(printf "%08d" $1)
nEvents=0
if [[ "$2" != "" ]]; then
    nEvents="$2"
fi

mvtxInputFiles="{"
for i in {0..5}
do
  mvtxInputFiles+="\"${inputListPath}/mvtx${i}_${runNumber}.list\","
done
mvtxInputFiles=${mvtxInputFiles::-1}
mvtxInputFiles+="}"

echo running: runCombiner_MVTX_only.sh 
sh ${fileFinderPath}/mvtx_makelist.sh $simpleRunNumber
root.exe -q -b Fun4All_Mvtx_Combiner.C\($nEvents,${mvtxInputFiles}\)
echo Script done
