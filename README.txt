Uzair Akram
Lab 4
The LAB demonstrates TCP connection; 3 way Handshake and  TCP close, and transfer a 1024 byte from client to server.

USAGE:

compile using:
gcc -o server server.c
gcc -o client client.c

run using:
./server <port number>
./client <port number>

port numbers must be same.

***The client takes as input filename: data.txt***
***The file data.txt is provided***

The file being transferred by client is named: data.txt
The file being transferred to server is named: data_sent.txt
The data from data.txt is copied to data_sent.txt

To view copied data Use:
Vim data_sent.txt --to view in text editor

To view in terminal Use:
cat data_sent.txt
Or 
cat < data_sent.txt


