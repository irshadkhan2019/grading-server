MClients=$1
loopNum=$2
sleepTimeSeconds=$3

touch data.txt > data.txt
touch data2.txt > data2.txt

for((i=1;i<=MClients;i++));do
    ./loadtest.sh $i $loopNum $sleepTimeSeconds
done


cat data.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "avgtime vs M" -X "M(Number of clients)" -Y "Avg time (in ms)" -r 0.25> ./avgtime.png
cat data2.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "througput vs M" -X "M(Number of clients)" -Y "Throughput" -r 0.25> ./throughput.png