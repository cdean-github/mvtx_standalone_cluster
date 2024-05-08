# MVTX Fun4All processing

## Building the analysis modules

A package should be built before running the scripts, or remove any mention of these packages in the Fun4All macros so you can just process the data. A handy bash alias to build the package is
```bash
alias buildThisProject="mkdir build && cd build && ../autogen.sh --prefix=$MYINSTALL && make && make install && cd ../"
```
Remember to have your work environment set up to allow local software builds with something like what I keep in my `.bash_profile`
```bash
source /opt/sphenix/core/bin/sphenix_setup.sh -n new
export SPHENIX=/sphenix/u/cdean/sPHENIX
export MYINSTALL=$SPHENIX/install
export LD_LIBRARY_PATH=$MYINSTALL/lib:$LD_LIBRARY_PATH
export ROOT_INCLUDE_PATH=$MYINSTALL/include:$ROOT_INCLUDE_PATH
source /opt/sphenix/core/bin/setup_local.sh $MYINSTALL
```

## Running over the data

There are 4 macros, each with an associated shell script and condor submission script to quickly batch process data.

The macros are:

1. Fun4All_Mvtx_Combiner.C
2. Fun4All_Tracker_Combiner.C
3. Fun4All_Silicon_Analyser.C
4. Fun4All_writeHit.C

### Fun4All_Mvtx_Combiner.C

This is the original macro. It will take raw MVTX evt (PRDF) files, combine them into single Fun4All events, convert them into TrkrHitSets then run our clustering algorithm on the hits. It can then execute the module ```mvtx_standalone_cluster``` which can write out nTuples of hits (or clusters depending on which code you uncomment in ```mvtx_standalone_cluster.cc```) and MVTX event displays to be used in our online event display, located at [https://www.sphenix.bnl.gov/edisplay/](https://www.sphenix.bnl.gov/edisplay/).

To run this, first create a folder under ```mvtx_standalone``` where the condor script lives
```bash
mkdir -p mvtx_standalone/input_lists
```
then you can process a single run with the provided shell script
```bash
./runCombiner_Mvtx_Combiner.sh <run number> <number of events>
```
(Note: if the number of events is not set, all events will be processed)

where the run number is the files run number, found in either the run data base or under ```/sphenix/lustre01/sphnxpro/commissioning/MVTX/cosmics/``` (N.B. this points to the cosmic ray data area, not the collision area, please alter ```mvtx_standalone/mvtx_makelist.sh``` to point to the beam area on lustre if needed).

You can also use condor to batch process several runs. make a file called ```list.runs``` in the folder ```mvtx_standalone``` with a new run on each line. Submit the condor job with
```
condor_submit myCondor.job
```
from the folder ```mvtx_standalone```

### Fun4All_Tracker_Combiner.C

This is the second macro and was used to generate event displays with the MVTX, INTT, TPC and TPOT when we were taking cosmic data. It probably has little use until we have reconstructed tracks. When they exist, the package will be updated to be a full event display

### Fun4All_Silicon_Analyser.C

This takes DSTs from the central production that contain Raw Hits from the MVTX and INTT (and TPOT if available). This will convert the raw hits into tracker hits, tracker clusters, tracklets and vertices (however the last two don't work well as we're still tuning the seeding and vertexing without the magnetic field). You can disable the seeding and vertexing in the macro by setting
```c++
bool runSeeding = false;
```
This macro will write out new DSTs which I've been storing under 
```
/sphenix/tg/tg01/commissioning/MVTX/beam/
```
Please be mindful of the output path that the DSTs will write to, set using the string
```c++
  string outpath = "/sphenix/tg/tg01/commissioning/MVTX/beam/20240507_ana.416Build";
```
This macro doesn't take a run number as an argument but takes a single DST input file (as the runs are very long, it will take a while to process each event in turn)

This module will also write an nTuple of hits (or clusters depending on which code you uncomment in ```mvtx_standalone_cluster.cc```). It will want to write the nTuples to a subfolder called ```hitTrees``` (just for neatness) under the directory where the macro lives. The output is set via the function call
```c++
myTester->writeFile("hitTrees/TTree_" + outtrailer);
```

You can run over a single run in a similar fastion to Fun4All_Mvtx_Combiner.C by doing
```bash
./runCombiner_Silicon_Analyser.sh <raw hit DST> <number of events>
```

You can also use the provided condor script under ```silicon_analyzer``` with a list of DSTs to process multiple files at once. There's also another shell script called ```checkForRuns.sh``` which will take a list of processed runs which have RawHit DSTs then check the MVTX PRDF file location to see if the MVTX was included in that run. A list.run file will be generated only for runs where the MVTX also took data 
