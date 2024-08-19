#!/bin/bash
set -e
# XVFB='xvfb-run --auto-servernum --server-args="-screen 0, 1280x1024x24"'
main_path=$PWD
start=`date +%s`
 

function end_time(){
    end=`date +%s`
    runtime=$((end-start))
    echo -e "\nTotal RunTime to run raptor_run.sh: $runtime">>results.log
    echo "Peak Memory Usage: 117360">>results.log
    echo "ExecEndTime: $end">>results.log
    raptor --version>>results.log
}

function parse_cga(){
    cd $main_path
    tail -n100 ./results_dir/raptor.log > ./results_dir/raptor_tail.log
    timeout 15m python3 ../../../../../parser.py ./results_dir/results.log ./results_dir/raptor_perf.log
    mv CGA_Result.json ./results_dir
}

command -v raptor >/dev/null 2>&1 && raptor_path=$(which raptor) || { echo >&2 echo "First you need to source Raptor"; end_time
parse_cga exit; }
lib_fix_path="${raptor_path:(-11)}"
library=${raptor_path/$lib_fix_path//share/raptor/sim_models}

#removing and creating raptor_testcase_files
#rm -fR $PWD/results_dir
[ ! -d $PWD/results_dir ] && mkdir $PWD/results_dir
[ -d $PWD/results_dir ] && touch $PWD/results_dir/CGA_Result.json
cd $main_path
[ -f CGA_Result_default.json ] && cp CGA_Result_default.json ./results_dir/CGA_Result.json
[ -d $PWD/results_dir ] && cd $PWD/results_dir

echo "ExecStartTime: $start">results.log
echo "Domain of the design: Unit Level Test">>results.log
# Check if parameters were passed as command line arguments
reg_id="23"
timeout="90"
synth_stage=""
mute_flag=""
if [[ $# -eq 6 ]]; then
  reg_id=$1
  timeout=$2
  post_synth_sim=$3
  device=$4
  synth_stage=$5
  mute_flag=$6
else
  if [[ $1 == "load_toolconf" ]]; then
      # Load parameters from tool.conf file
      source $main_path/../../tool.conf
  elif [[ $1 == "clean" ]]; then
      cd $main_path
      PIDS=$(lsof +D "results_dir" | awk 'NR>1 {print $2}' | uniq)
      for PID in $PIDS; do
          if kill -0 $PID 2>/dev/null; then
              echo "Attempting to terminate process $PID..."
              kill $PID
              sleep 1
          if kill -0 $PID 2>/dev/null; then
              echo "Process $PID did not terminate gracefully. Forcing termination."
              kill -9 $PID 2>/dev/null || echo "Could not force terminate process $PID. It may have already exited."
          fi
          else
              echo "Process $PID already terminated."
          fi
      done
      rm -rf cksums.md5 newsum.md5 raptor_tcl.tcl results_dir/
      echo "Files cleaned"
      exit 0
    else
      echo "Using paramters defined in raptor_run.sh"
    fi
fi

if [ -z $1 ]; then
    echo "RegID: $reg_id">>results.log
else
    echo "RegID: $1">>results.log
fi

if [ -z $2 ]; then
    echo "timeout: $timeout">>results.log
else
    timeout=$2
    echo "timeout: $2">>results.log
fi

function compile () {

    module unload synopsys/1.0
    
    cd $main_path/results_dir
    echo $PWD

    timeout+='m'
   
    timeout 2m xvfb-run --auto-servernum --server-args="-screen 0, 1280x1024x24" raptor --compiler dummy $mute_flag --script ../design_runs.tcl 2>&1 | tee -a results.log
    if [ ${PIPESTATUS[0]} -eq 124 ]; then
        echo -e "\nERROR: TIM: Design Compilation took $timeout. Exiting due to timeout">>raptor.log
        cat raptor.log >> results.log
        end_time
        parse_cga
        exit
    fi 
  
}

compile 
cat raptor.log >> results.log
echo -e "\n\n#########Raptor Performance Data#########" >> results.log
cat raptor_perf.log >> results.log
echo -e "#############################################\n" >> results.log

end_time
parse_cga
