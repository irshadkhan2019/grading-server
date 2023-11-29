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
# touch data.txt > data.txt
# touch data2.txt > data2.txt

declare -a pids

for((i=1;i<=numClients;i++));do
    touch results_$i.txt > results_$i.txt
    ./simple-client.o localhost 9999 program.c $loopNum $sleepTimeSeconds >> "results_$i.txt" &
    pids+=($!)
done


# Wait for all background processes to finish
for pid in "${pids[@]}"; do
    wait "$pid"
done

# Calculate overall throughput and average response time
total_throughput=0
total_response_time=0
response_time=0;
total_response_time=0;
avg_response_time=0;
througput=0;
total_throughput=0;
total_loop_time=0;

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
    # total_throughput=$(echo "$total_throughput + $throughput" | bc)
    total_loop_time=$(echo "$looptime + $total_loop_time" | bc)

    # total_response_time=$((total_response_time + avg_time))
    # throughput=$(awk -v loopNum="$loopNum" '/Time take for completing loop :/ {thrpt = loopNum/($7/1000); print thrpt}' "results_$i.txt")
    # total_throughput=$((total_throughput + throughput))
done
    avg_response_time=$(echo "$total_response_time / $total_success_res" | bc)
    total_throughput=$(echo "scale=2; $total_success_res / $total_loop_time" | bc)
    echo $total_avg_time
    echo $total_success_res
    echo $avg_response_time
    echo $total_loop_time
    echo $total_throughput

# througput_per_client=$(echo "$total_througput + $throughput" | bc)
# througput_per_client=$total_throughput / $numClients
echo "$numClients $avg_response_time" >> "data.txt"
echo "$numClients $total_throughput" >> "data2.txt"    


  
# # avg_time=$(awk '/AVG response time:/ {sum += $4; count++} END {if (count > 0) print "Total AVG response time:", sum/count, "msec"}' results.txt)
# avg_time=$(awk -v loopNum="$loopNum"'/AVG response time:/ {sum += $4; count++} END {if (count > 0) print sum/loopNum}' results.txt)
# echo "$i $avg_time" >> "data.txt"

# calculate avg response time
#!/bin/bash

# avg_time=$(awk '/AVG response time:/ {sum += $4; count++} END {if (count > 0) print "Total AVG response time:", sum/count, "msec"}' results.txt)
# echo $avg_time
# throughput=$(awk -v loopNum="$loopNum" 'BEGIN{ i=1 } /Time take for completing loop :/ {thrpt = loopNum/($7/1000);print thrpt;print i " " thrpt >> "data2.txt" ;++i; } END {}' results.txt)
# # throughput=$(awk -v loopNum="$loopNum" 'BEGIN { i=1 } /Time take for completing loop :/ { thrpt = loopNum/($7/1000); print thrpt; print i " " thrpt >> "data2.txt"; ++i; } END {}' results.txt)

# echo $throughput
# # # Plot the results

# cat data.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "avgtime vs M" -X "M(Number of clients)" -Y "Avg time" -r 0.25> ./plot.png
# cat data2.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "througput vs M" -X "M(Number of clients)" -Y "Throughput" -r 0.25> ./plot2.png