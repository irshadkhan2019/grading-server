#!/bin/bash

# Check if all arguments are provided
if [ "$#" -ne 3 ]; then
    echo "Usage: ./loadtest.sh <numClients> <loopNum> <sleepTimeSeconds>"
    exit 1
fi

numClients=$1
loopNum=$2
sleepTimeSeconds=$3
total_avg_time=0
total_success_res=0

touch results.txt > results.txt

declare -a pids

for ((i=1; i<=numClients; i++)); do
    touch results_$i.txt > results_$i.txt
    # echo "calculating for results_$i.txt results_$i.txt "
    # Start CPU utilization monitoring using vmstat
    vmstat 1 10 > vmstat_$i.log &

    # Start collecting NLWP information using ps command
    ps -eLf | awk '$10 ~ /^\.\/server/'|head -1 | awk '{print $6 -1}' > ps_$i.log & 

    # Start the client in the background
    ./client localhost 9999 program.c $loopNum $sleepTimeSeconds new >> "results_$i.txt" &

    pids+=($!)
done

# Wait for all background processes to finish
for pid in "${pids[@]}"; do
    wait "$pid"
done

# Calculate overall throughput and average response time
total_throughput=0
total_response_time=0
total_response_time=0
total_loop_time=0
total_avg_cpu_utilization=0
total_avg_active_threads=0

echo "calculating"

for ((i=1; i<=numClients; i++)); do
   
    avg_time=$(awk '/AVG response time:/ {sum += $4; } END {print sum}' "results_$i.txt")
    
    looptime=$(awk '/Time take for completing loop :/ {time = $7/1000;} END {print time}' "results_$i.txt")
 
    successfull_response=$(awk '/Successful Response:/ {sum += $3; } END {print sum}' "results_$i.txt")

    total_avg_time=$(echo "$total_avg_time + $avg_time" | bc)
    total_success_res=$(echo "$total_success_res + $successfull_response" | bc)
    response_time=$(echo "$successfull_response * $avg_time" | bc)
    total_response_time=$(echo "$total_response_time + $response_time" | bc)
    throughput=$(echo "scale=2; $successfull_response / $looptime" | bc)
    total_loop_time=$(echo "$looptime + $total_loop_time" | bc)
    
    # Calculate CPU utilization by averaging the values in the vmstat log
    avg_cpu_utilization=$(tail -n 1 vmstat_$i.log | awk '{print 100 - $15}') 
    total_avg_cpu_utilization=$(echo "$total_avg_cpu_utilization + $avg_cpu_utilization" | bc)
    
    # Calculate the average number of active threads (NLWP) by counting the lines in the ps log
    avg_active_threads=$(cat ps_$i.log)
    total_avg_active_threads=$(echo "$total_avg_active_threads + $avg_active_threads" | bc)

    total_throughput=$(echo "$throughput + $total_throughput" | bc)
    
done

  echo "Done calculating for results_$i.txt "

avg_response_time=$(echo "$total_response_time / $total_success_res" | bc)

total_avg_cpu_utilization=$(echo "scale=2; $total_avg_cpu_utilization / $numClients" | bc)
total_avg_active_threads=$(echo "scale=2; $total_avg_active_threads / $numClients" | bc)

echo "Total AVG response time: $avg_response_time msec"
echo "Total Successful Responses: $total_success_res"
echo "Total Time taken for completing loop: $total_loop_time msec"
echo "Total Throughput: $total_throughput"
echo "Total AVG CPU Utilization: $total_avg_cpu_utilization%"
echo "Total AVG Active Threads (NLWP): $total_avg_active_threads"

# Save the results to a file
echo "$numClients $avg_response_time" >> "data.txt"
echo "$numClients $total_throughput" >> "data2.txt"
echo "$numClients $total_avg_cpu_utilization" >> "data3.txt"
echo "$numClients $total_avg_active_threads" >> "data4.txt"
