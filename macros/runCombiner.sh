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

fileFinderPath=tracker_combiner
inputListPath=${fileFinderPath}/input_lists

source /opt/sphenix/core/bin/setup_local.sh $MYINSTALL

simpleRunNumber=$1
runNumber=$(printf "%08d" $1)
nEvents=0
if [[ "$2" != "" ]]; then
    nEvents="$2"
fi

gl1InputFile="{\"${inputListPath}/gl1_${runNumber}.list\"}"
tpotInputFile="{\"${inputListPath}/tpot_${runNumber}.list\"}"

mvtxInputFiles="{"
for i in {0..5}
do
  mvtxInputFiles+="\"${inputListPath}/mvtx${i}_${runNumber}.list\","
done
mvtxInputFiles=${mvtxInputFiles::-1}
mvtxInputFiles+="}"

inttInputFiles="{"
for i in {0..7}
do
  inttInputFiles+="\"${inputListPath}/intt${i}_${runNumber}.list\","
done
inttInputFiles=${inttInputFiles::-1}
inttInputFiles+="}"

tpcInputFiles="{"
for i in {00..23}
do
  tpcInputFiles+="\"${inputListPath}/tpc${i}_${runNumber}.list\","
done
tpcInputFiles=${tpcInputFiles::-1}
tpcInputFiles+="}"

echo running: runCombiner.sh
sh ${fileFinderPath}/make_cosmics.sh $simpleRunNumber
root.exe -q -b Fun4All_Tracker_Combiner.C\($nEvents,${gl1InputFile},${mvtxInputFiles},${inttInputFiles},${tpcInputFiles},${tpotInputFile}\)
#root.exe -q -b Fun4All_Tracker_Combiner.C\($nEvents,\"${inputListPath}/gl1_${runNumber}.list\",\"${inputListPath}/mvtx0_${runNumber}.list\",\"${inputListPath}/mvtx1_${runNumber}.list\",\"${inputListPath}/mvtx2_${runNumber}.list\",\"${inputListPath}/mvtx3_${runNumber}.list\",\"${inputListPath}/mvtx-4_${runNumber}.list\",\"${inputListPath}/mvtx5_${runNumber}.list\",\"${inputListPath}/intt0_${runNumber}.list\",\"${inputListPath}/intt1_${runNumber}.list\",\"${inputListPath}/intt2_${runNumber}.list\",\"${inputListPath}/intt3_${runNumber}.list\",\"${inputListPath}/intt4_${runNumber}.list\",\"${inputListPath}/intt5_${runNumber}.list\",\"${inputListPath}/intt6_${runNumber}.list\",\"${inputListPath}/intt7_${runNumber}.list\",\"${inputListPath}/tpc0_${runNumber}.list\",\"${inputListPath}/tpc1_${runNumber}.list\",\"${inputListPath}/tpc2_${runNumber}.list\",\"${inputListPath}/tpc3_${runNumber}.list\",\"${inputListPath}/tpc4_${runNumber}.list\",\"${inputListPath}/tpc5_${runNumber}.list\",\"${inputListPath}/tpc6_${runNumber}.list\",\"${inputListPath}/tpc7_${runNumber}.list\",\"${inputListPath}/tpc8_${runNumber}.list\",\"${inputListPath}/tpc9_${runNumber}.list\",\"${inputListPath}/tpc10_${runNumber}.list\",\"${inputListPath}/tpc11_${runNumber}.list\",\"${inputListPath}/tpc12_${runNumber}.list\",\"${inputListPath}/tpc13_${runNumber}.list\",\"${inputListPath}/tpc14_${runNumber}.list\",\"${inputListPath}/tpc15_${runNumber}.list\",\"${inputListPath}/tpc16_${runNumber}.list\",\"${inputListPath}/tpc17_${runNumber}.list\",\"${inputListPath}/tpc18_${runNumber}.list\",\"${inputListPath}/tpc19_${runNumber}.list\",\"${inputListPath}/tpc20_${runNumber}.list\",\"${inputListPath}/tpc21_${runNumber}.list\",\"${inputListPath}/tpc22_${runNumber}.list\",\"${inputListPath}/tpc23_${runNumber}.list\",\"${inputListPath}/tpot_${runNumber}.list\"\)
echo Script done
