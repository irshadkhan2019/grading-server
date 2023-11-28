MClients=$1
loopNum=$2
sleepTimeSeconds=$3

touch data.txt > data.txt
touch data2.txt > data2.txt
touch data3.txt > data3.txt
touch data4.txt > data4.txt

for((i=1;i<=MClients;i++));do
    ./loadtest.sh $i $loopNum $sleepTimeSeconds 
done


cat data.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "avgtime vs M" -X "M(Number of clients)" -Y "Avg time (in ms)" -r 0.25> ./avgtime.png
cat data2.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "througput vs M" -X "M(Number of clients)" -Y "Throughput" -r 0.25> ./throughput.png
cat data3.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "cpu utilization vs M" -X "M(Number of clients)" -Y "cpu utilization" -r 0.25> ./cpu_util.png
cat data4.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "active threads vs M" -X "M(Number of clients)" -Y "Active threads" -r 0.25> ./active_threads.png

rm -r vmstat_*.log
rm -r results_*.txt
rm -r ps_*.log
rm -r out*
rm -r file*
rm -r diff*
rm -r exec*
rm -r err*


 