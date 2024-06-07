#!/bin/bash

if [ $# -eq 0 ]
  then
    echo "No arguments supplied."
    echo "You must at least specify a run number and run type"
    echo "The third argument is the number of events. If no argument is set, all events will be processed."
    exit 1
fi

# run_number=$(printf "%08d" $1)
run_number=$1
run_type="\"${2}\""
nEvents=-1
if [[ "$3" != "" ]]; then
    nEvents="$3"
fi

source /opt/sphenix/core/bin/sphenix_setup.sh -n new
export INSTALLDIR=/sphenix/user/tmengel/MVTX/MvtxHitMap/mvtx_standalone_cluster/install
source /opt/sphenix/core/bin/setup_local.sh $INSTALLDIR

OUTPUTBASE=/sphenix/user/tmengel/MVTX/MvtxHitMap/mvtx_standalone_cluster/macros/mvtx_hitmap/rootfiles
# mkdir subdirectory with run number
OUTPUTDIR=${OUTPUTBASE}/$(printf "%08d" $run_number)_${2}
mkdir -p $OUTPUTDIR
OUTPUTDIR="\"${OUTPUTDIR}/${2}_${run_number}__mvtx_chip_occ_hitmap.root\""

echo "Run number: $run_number"
echo "Run type: $run_type"
echo "Number of events: $nEvents"
echo "Output directory: $OUTPUTDIR"

cd /sphenix/user/tmengel/MVTX/MvtxHitMap/mvtx_standalone_cluster/macros
root -l -q -b runCombiner_Mvtx_hitmap.sh.C\($run_number,$run_type,$nEvents,$OUTPUTDIR\)
echo "Script done"
