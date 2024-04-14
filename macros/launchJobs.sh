#!/bin/bash

submissionScript=myCondor.job

for i in 38402 38934 38935 38936 38941 38943 38944 38945 39203 39204 39205 39206 39207
  do
    lastRunNumber=($(grep Argument $submissionScript | awk '{print $3}' | head -n 1))
    sed -i -e s/$lastRunNumber/$i/g $submissionScript
    condor_submit $submissionScript
    echo "Job submitted for run $i"
  done
