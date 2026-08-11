#!/bin/bash
GRN='\033[1;32m'
BLU='\033[1;34m'
NORM='\033[0m'

buildfolder="build"
sourcefolder=$(realpath .)
tests=()

intervals=(2 4 8)
segments=(2 4 8 16 32 64 128 256 512 1024)
iterations=(2 4 8 16)
functions=(tanh sigmoid silu gaussian sinc cos exp softsign)
for interval in "${intervals[@]}"
do
  for segment in "${segments[@]}"
  do
    for iteration in "${iterations[@]}"
    do
      for funcnum in "${functions[@]}"
      do
        tests+=("sysc_approx_fp_${interval}_${segment}_${iteration}#${funcnum}")
      done
    done
  done
done

today=$(date +"%Y-%m-%d-%H.%M")
testfolder_base=test/$today
klee_args=(
		"--libcxx"
		"--libc=uclibc"
		"-posix-runtime"
		"-only-output-states-covering-new"
		"-max-memory=40000"
		"--max-time=12h"
		"--watchdog"
		"--search=bfs"
		#"-exit-on-error"
		"--const-array-threshold=4"
		"-symsim"
		"--symsim-local"
		"--symsim-global"
		"--symsim-solver"
                "--symsim-prefix=source/duts/"
                "--symsim-prefix=source/benches/"
)

echo "Today is $today, writing to $testfolder_base"
#	rm -r "$testfolder/*" 2> /dev/null
mkdir -p $testfolder_base 2> /dev/null

make -j5 -s -C $buildfolder clean
i=0 #lol
j=0
for test in ${tests[@]}
do
  if [ $j -ge 4 ]
  then
    for((k=0;k<5;k++)); do
	echo "waiting for PID ${klee_pid[${k}]} (${tests[$((i-j))]})..."
      wait "${klee_pid[${k}]}"
      echo -e "${GRN}${tests[$((i-j))]} finished.$NORM"
      unset 'klee_pid[${k}]'
      j=$[j - 1]
    done
    j=0
  fi
	base_name=$(echo $test | cut -d "#" -f1)
	subtype=$(echo $test | cut -d "#" -s -f2)
	testfolder=$testfolder_base/$test
	mkdir $testfolder
	echo "Building testbench_$base_name"
	make -j5 -C $buildfolder testbench_$base_name --no-print-directory
	echo -e "${BLU}Running test $base_name ($subtype)$NORM"
	klee_target_folder[${i}]="$testfolder"
	#rm -rf klee_folder[${i}]
				args="${klee_args[*]} $buildfolder/testbench_${base_name}"
	echo "klee --output-dir=${klee_target_folder[${i}]}/klee-run ${args} $subtype > $testfolder/run.log"
	{ time klee --output-dir=${klee_target_folder[${i}]}/klee-run ${args} $subtype ; } > "$testfolder/run.log" 2>&1 &
	 klee_pid[${j}]=$!
	sleep 1
	 echo "$base_name ($subtype) running as ${klee_pid[${j}]} into ${klee_folder[${i}]}"
#	echo -e "${GRN}${tests[${i}]} finished.$NORM"
	i=$[i + 1]
	j=$[j + 1]
done

for ((i=0;i<${#klee_pid[@]};i++)); do
		echo "waiting for PID ${klee_pid[${i}]} (${tests[${i}]})..."
		wait "${klee_pid[${i}]}"
		echo -e "${GRN}${tests[${i}]} finished.$NORM"
		# copy temporary result folder into outputfolder
		#mv "${klee_folder[${i}]}" "${klee_target_folder[${i}]}"
done

end_stats=$testfolder_base/klee-stat.log

klee-stats --print-more "$testfolder_base"/* > "$end_stats"
#tail $testfolder_base/*/run.log >> $testfolder_base/klee-stat.log

for ((i=0;i<${#tests[@]};i++)); do
		echo >> "$end_stats"
		echo "${klee_target_folder[${i}]}/run.log found errors: " >> "$end_stats"
		cat "${klee_target_folder[${i}]}/run.log" | grep ERROR >> "$end_stats"
		tail -n 8 "${klee_target_folder[${i}]}/run.log" >> "$end_stats"
done

cat $testfolder_base/klee-stat.log
