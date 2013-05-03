#!/bin/bash
# Launch n genroot jobs processing the same run, then create a TChain 
# combining the output

print_usage(){
    echo "Usage: ./paragenroot.sh <nthreads> [<genroot options>] <raw datafile>"
    exit 1
}

#make sure we have the right number of args
[ $# -gt 1 ] || print_usage 
#make sure the second arg is a  number
[ "$1" -eq "$1" ] 2>/dev/null || print_usage 

nthreads=$1
shift
rawfile=$(echo "$@" | sed 's/ /\n/g' | tail -n 1) 
genroot_args=""
while [ $# -gt 1 ] ; do
    genroot_args="$genroot_args $1"
    shift
done

#determine an appropriate name for the rootfile
rootbase=$(basename $rawfile ".out")
rootbase=$(basename $rootbase ".000")
rootbase=$(basename $rootbase ".out.gz")

#determine the number of events in the run
echo "Determining total number of events in raw data..."

nevents=$(./run_info -a n $rawfile | grep -w "events" | sed -e 's/^.*events \([0-9]*\) .*$/\1/g') 

if [ $? -ne 0 ] ; then
    echo "There was an error opening the raw datafile $rawfile...aborting!"
    exit 2
fi

evperthread=$(( nevents/nthreads ))

echo "Processing $nevents events with $nthreads threads ($evperthread ev/thread)"
njobs=0
jobs=""
rootfiles=""

while [ $njobs -lt $nthreads ] ; do
    min="$(( njobs*evperthread ))"
    max="$(( min+evperthread ))"
    range="--min $min"
    jobnum=$njobs;
    if [ $njobs -lt $(( nthreads-1 )) ] ; then
	range="$range --max $max"
    fi
    #0-pad the jobnum part
    while [ ${#jobnum} -lt 2 ] ; do jobnum="0$jobnum" ; done
    rootfile="${rootbase}_${jobnum}.root"
    rootfiles="$rootfiles $rootfile"
    logfile="${rootbase}_${jobnum}.log"
    logfiles="$logfiles $logfile"
    echo "Events $min to $max logged in $logfile"
    ./genroot $genroot_args $range --rootfile $rootfile $rawfile >$logfile &  
    jobs="$jobs $!"
    (( njobs++ ))
done

for jobs in $jobs ; do 
    wait $job
done
 
#find the directory where the footfiles are stored
rootdir=$(grep -h "Saving output to file " $logfile | sed -e 's#^.* \(.*\)/.*\.root\.*$#\1#g')
echo "Root files are stored in $rootdir"

olddir=$(pwd)
cd $rootdir

#chain the output root files together
macro="$(pwd)/chainroot.C"
cat > $macro <<EOF
{
  TFile* f = new TFile("$rootbase.root","RECREATE");
  TChain* c = new TChain("Events");
  c->Add("${rootbase}_*.root");
  f->cd();
  TTree* tree = c->CloneTree(-1,"fast");
  tree->Write();
  f->Close();  
}
EOF

echo "Chaining output root files..."
$olddir/daqroot -b -q $macro >/dev/null

#cleanup
rm -f $macro
rm -f $rootfiles
cd $olddir
rm -f $logfiles

echo "Done!"