Github-Repo :https://github.com/irshadkhan2019/grading-server
------------------------------------------------------------------------------------------------------------
Running Version 1:

 (A) Run Server :
 >> gcc -o server simple-server.c
 >> ./server <portnumber>
 
 (B) Running 1 client : 
 >> gcc -o client simple-client.c
 >> ./client localhost <port> <progrram_file> <loopnum> <sleepTimeSeconds>
 
 (C) To run performance test with multiple client ensure Server is running at port 9999 if using varyM.sh
 >> ./varyM.sh <NumberOfclients> <loopNum> <sleepTimeSeconds>

------------------------------------------------------------------------------------------------------------
Running Version 2:

 (A) Run Server :
 >> gcc -o server simple-server.c
 >> ./server <portnumber>
 
 (B) Running 1 client : 
 >> gcc -o client simple-client.c
 >> ./client localhost <port> <progrram_file> <loopnum> <sleepTimeSeconds>
 
 (C) To run performance test with multiple client ensure Server is running at port 9999 if using varyM.sh
 >> ./varyM.sh <NumberOfclients> <loopNum> <sleepTimeSeconds>
 
-----------------------------------------------------------------------------------------------------------

Running Version 3:
 
 (A) Run Server : 
 >> g++ -o server simple-server.cpp
 >> ./server <portnumber> <threads>
 
 (B) Running 1 client:
 >> gcc -o client simple-client.cpp
 >> ./client localhost <port> <program_file> <loopNum> <sleepTimeSeconds> 
 
 (C) To run performance test with multiple client ensure Server is running at port 9999 if using varyM.sh
 >> ./varyM.sh <NumberOfclients> <loopNum> <sleepTimeSeconds>
 
--------------------------------------------------------------------------------------------------------

Running Version 4:
 
 A) Run server :
 >> g++ -o server simple-server.cpp
 >> ./server <portnumber> <threads> 

 B)Running 1 client : 
 >> gcc -o client simple-client.c 
 >> ./client localhost <port> <program_file> <loopNum> <sleepTimeSeconds> <new|status> <tokenid>?optional   

 C)TO run performance test with multiple client ensure Server is running at port 9999 if using varyM.sh
 >> ./varyM.sh <NumberOfclients> <loopNum> <sleepTimeSeconds>   

---------------------------------------------------------------------------------------------------------

