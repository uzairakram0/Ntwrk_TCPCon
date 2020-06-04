#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
//data transfer sizes
#define size 1024
#define d 128
//Flag bit masks
#define FIN 0b0000000000000001
#define SYN 0b0000000000000010
#define RST 0b0000000000000100
#define PSH 0b0000000000001000
#define ACK 0b0000000000010000
#define URG 0b0000000000100000

struct tcp_hdr{
    unsigned short int src;
    unsigned short int des;
    unsigned int seq;
    unsigned int ack;
    unsigned short int hdr_flags;
    unsigned short int rec;
    unsigned short int cksum;
    unsigned short int ptr;
    unsigned int opt;
    char data[d];
};

//Prototypes
void calculateCksum(struct tcp_hdr *tcp_seg);
void showrcv(struct tcp_hdr tcp_seg, FILE *fp);
void showsnt(struct tcp_hdr tcp_seg, FILE *fp);
void outputTransferFile(char buff[d]);

int main(int argc,char **argv)
{
    char str[200];
    int listen_fd, conn_fd;
    struct sockaddr_in servaddr;
    char buffer[size];
    
    /*TCP Handshake structs*/
    struct tcp_hdr tcp_seg;
    struct tcp_hdr con_grant;       //connection granted
    struct tcp_hdr ack_seg;
    /*TCP Send data segment*/
    struct tcp_hdr snd_data;
    struct tcp_hdr data_ack;
    /*TCP close structs*/
    struct tcp_hdr cls_cli;
    struct tcp_hdr ack_cls;
    struct tcp_hdr cls_svr;
    struct tcp_hdr final_ack;
    
    unsigned short int headerlength = sizeof(tcp_seg);
    headerlength = headerlength << 11;
    
    srand (time(NULL));
    unsigned int seqnum = rand();
    
    /*remove the previous file, to avoid appending data*/
    remove("data_sent.txt");
    
    /*write to server.out file*/
    FILE *fp;
    fp = fopen("server.out", "w+");
 
    /*Print USAGE if ran without Server Port No.*/
    if(argc < 2 ){fprintf(stderr, "usage: %s <Port Number>\n", argv[0]); exit(1);}
    int port = atoi(argv[1]);
    
    /* AF_INET - IPv4 IP , Type of socket, protocol*/
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
 
    bzero(&servaddr, sizeof(servaddr));
 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(port); /* Pick another port number to avoid conflict with other students */
 
    /* Binds the above details to the socket */
	bind(listen_fd,  (struct sockaddr *) &servaddr, sizeof(servaddr));
	/* Start listening to incoming connections */
	listen(listen_fd, 10);
    
    printf("SERVER\n");
    fprintf(fp, "SERVER\n");
    
    /* Accepts an incoming connection */
    conn_fd = accept(listen_fd, (struct sockaddr*)NULL, NULL);
    
    printf("\n--HANDSHAKE--\n");
    fprintf(fp, "\n--HANDSHAKE--\n");
    
    /*HANDSHAKE*/
    //recieve handshake initializing segment
    recv(conn_fd, &tcp_seg, sizeof(tcp_seg), 0);
    showrcv(tcp_seg, fp);
    
    //create connection granted segment
    con_grant.src = port;
    con_grant.des = port;
    con_grant.seq = seqnum;
    con_grant.ack = tcp_seg.seq + 1;
    con_grant.hdr_flags = headerlength;
    con_grant.hdr_flags = con_grant.hdr_flags | SYN;
    con_grant.hdr_flags = con_grant.hdr_flags | ACK;
    con_grant.rec = 0;
    con_grant.cksum = 0;
    con_grant.ptr = 0;
    con_grant.opt = 0;
    calculateCksum(&con_grant);
    strcpy(con_grant.data, "");
    //send connection granted segement
    showsnt(con_grant, fp);
    send(conn_fd, &con_grant, sizeof(con_grant), 0);
    
    //recieve handshake initializing segment
    recv(conn_fd, &ack_seg, sizeof(ack_seg), 0);
    showrcv(ack_seg, fp);
    
    /*RECIEVE DATA*/
    printf("\n--DATA--\n");
    fprintf(fp, "\n--DATA--\n");
    
    //loop till all file seqments recieved
    for (int i =0; i < (size/d); i++){
        //recieve connection granted segment
        recv(conn_fd, &snd_data, sizeof(snd_data), 0);
        showrcv(snd_data, fp);

        outputTransferFile(snd_data.data);
        
        //send acknowledgement
        data_ack.src = port;
        data_ack.des = port;
        data_ack.seq = snd_data.ack;
        data_ack.ack = snd_data.seq + 1;
        data_ack.hdr_flags = headerlength;
        data_ack.hdr_flags = data_ack.hdr_flags | SYN;
        data_ack.hdr_flags = data_ack.hdr_flags | ACK;
        data_ack.rec = 0;
        data_ack.cksum = 0;
        data_ack.ptr = 0;
        data_ack.opt = 0;
        calculateCksum(&data_ack);
        strcpy(data_ack.data, "");
        //send tcp acknowledgement segment
        showsnt(data_ack, fp);
        send(conn_fd, &data_ack, sizeof(data_ack), 0);
    }
    
    
    printf("\n--CLOSE--\n");
    fprintf(fp, "\n--CLOSE--\n");
    
    /*CLOSING*/
    seqnum = rand();
    
    //recieve close request
    recv(conn_fd, &cls_cli, sizeof(cls_cli), 0);
    showrcv(cls_cli, fp);
    
    //create close acknowledgement
    ack_cls.src = port;
    ack_cls.des = port;
    ack_cls.seq = seqnum;
    ack_cls.ack = cls_cli.seq + 1;
    ack_cls.hdr_flags = headerlength;
    ack_cls.hdr_flags = ack_cls.hdr_flags | ACK;
    ack_cls.rec = 0;
    ack_cls.cksum = 0;
    ack_cls.ptr = 0;
    ack_cls.opt = 0;
    calculateCksum(&ack_cls);
    strcpy(ack_cls.data, "");
    //send acknowledgement for closee
    showsnt(ack_cls, fp);
    send(conn_fd, &ack_cls, sizeof(ack_cls), 0);
    
    sleep(1);
    //create server close segment
    cls_svr.src = port;
    cls_svr.des = port;
    cls_svr.seq = ++seqnum;
    cls_svr.ack = cls_cli.seq + 1;
    cls_svr.hdr_flags = headerlength;
    cls_svr.hdr_flags = cls_svr.hdr_flags | FIN;
    cls_svr.rec = 0;
    cls_svr.cksum = 0;
    cls_svr.ptr = 0;
    cls_svr.opt = 0;
    calculateCksum(&cls_svr);
    strcpy(ack_cls.data, "");
    //send the server side close request
    showsnt(cls_svr, fp);
    send(conn_fd, &cls_svr, sizeof(cls_svr), MSG_WAITALL);
    
    //recieve acknowledgemnt
    recv(conn_fd, &final_ack, sizeof(final_ack), MSG_WAITALL);
    showrcv(final_ack, fp);
    
    fclose(fp);
    close (conn_fd); //close the connection
    
    return 0;
}

void calculateCksum(struct tcp_hdr *tcp_seg){
    
    unsigned short int cksum_arr[12];
    unsigned int i, sum  = 0, cksum;
    
    memcpy(cksum_arr, &tcp_seg, 24); //Copying 24 bytes
    
    for (i=0;i<12;i++)               // Compute sum
        sum = sum + cksum_arr[i];
    
    cksum = sum >> 16;              // Fold once
    sum = sum & 0x0000FFFF;
    sum = cksum + sum;
    cksum = sum >> 16;             // Fold once more
    sum = sum & 0x0000FFFF;
    cksum = cksum + sum;
    
    /* XOR the sum for checksum */
    //printf("Checksum Value: 0x%04X\n", (0xFFFF^cksum));
    
    tcp_seg->cksum = (0xFFFF^cksum);
}

void showrcv(struct tcp_hdr tcp_seg, FILE *fp){
    /*write to console*/
    printf("\nRecieving\n");
    printf("src:0x%04X\n", tcp_seg.src); // Printing all values
    printf("des:0x%04X\n", tcp_seg.des);
    printf("seq:0x%08X\n", tcp_seg.seq);
    printf("ack:0x%08X\n", tcp_seg.ack);
    printf("hdr:0x%04X\n", tcp_seg.hdr_flags);
    printf("rec:0x%04X\n", tcp_seg.rec);
    printf("cksum:0x%04X\n", tcp_seg.cksum);
    printf("ptr:0x%04X\n", tcp_seg.ptr);
    printf("opt:0x%08X\n", tcp_seg.opt);
    //printf("data:%s\n", tcp_seg.data);
    /*write to FILE*/
    fprintf(fp, "\nRecieving\n");
    fprintf(fp, "src:0x%04X\n", tcp_seg.src);
    fprintf(fp, "des:0x%04X\n", tcp_seg.des);
    fprintf(fp, "seq:0x%08X\n", tcp_seg.seq);
    fprintf(fp, "ack:0x%08X\n", tcp_seg.ack);
    fprintf(fp, "hdr:0x%04X\n", tcp_seg.hdr_flags);
    fprintf(fp, "rec:0x%04X\n", tcp_seg.rec);
    fprintf(fp, "cksum:0x%04X\n", tcp_seg.cksum);
    fprintf(fp, "ptr:0x%04X\n", tcp_seg.ptr);
    fprintf(fp, "opt:0x%08X\n", tcp_seg.opt);
    //fprintf(fp, "data:%s\n", tcp_seg.data);
}

void showsnt(struct tcp_hdr tcp_seg, FILE *fp){
    /*write to console*/
    printf("\nSending\n");
    printf("src:0x%04X\n", tcp_seg.src); // Printing all values
    printf("des:0x%04X\n", tcp_seg.des);
    printf("seq:0x%08X\n", tcp_seg.seq);
    printf("ack:0x%08X\n", tcp_seg.ack);
    printf("hdr:0x%04X\n", tcp_seg.hdr_flags);
    printf("rec:0x%04X\n", tcp_seg.rec);
    printf("cksum:0x%04X\n", tcp_seg.cksum);
    printf("ptr:0x%04X\n", tcp_seg.ptr);
    printf("opt:0x%08X\n", tcp_seg.opt);
    //printf("data:%s\n", tcp_seg.data);
    /*write to FILE*/
    fprintf(fp, "\nSending\n");
    fprintf(fp, "src:0x%04X\n", tcp_seg.src);
    fprintf(fp, "des:0x%04X\n", tcp_seg.des);
    fprintf(fp, "seq:0x%08X\n", tcp_seg.seq);
    fprintf(fp, "ack:0x%08X\n", tcp_seg.ack);
    fprintf(fp, "hdr:0x%04X\n", tcp_seg.hdr_flags);
    fprintf(fp, "rec:0x%04X\n", tcp_seg.rec);
    fprintf(fp, "cksum:0x%04X\n", tcp_seg.cksum);
    fprintf(fp, "ptr:0x%04X\n", tcp_seg.ptr);
    fprintf(fp, "opt:0x%08X\n", tcp_seg.opt);
    //fprintf(fp, "data:%s\n", tcp_seg.data);
}

void outputTransferFile(char buff[d]){
    FILE *fp;
    fp = fopen("data_sent.txt", "a+");
    
    fprintf(fp, "%s", buff);
    
    fclose(fp);
}

