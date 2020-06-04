#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
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
void readTransferFile(char buff[size]);

int main(int argc,char **argv)
{
    int sockfd, n;
    int len = sizeof(struct sockaddr);
    struct sockaddr_in servaddr;
    char buffer[size];
    
    /*TCP Handshake structs*/
    struct tcp_hdr tcp_seg;
    struct tcp_hdr con_grant;
    struct tcp_hdr ack_seg;
    /*TCP Send data segment*/
    struct tcp_hdr snd_data;
    struct tcp_hdr data_ack;
    /*TCP close structs*/
    struct tcp_hdr cls_cli;
    struct tcp_hdr ack_cls;
    struct tcp_hdr cls_svr;
    struct tcp_hdr final_ack;
    
    srand (time(NULL));
    unsigned int seqnum = rand();
    
    unsigned short int headerlength = sizeof(tcp_seg);
    headerlength = headerlength << 10;
    
    /*Write to client.out file*/
    FILE *fp;
    fp = fopen("client.out", "w+");
    
    /* AF_INET - IPv4 IP , Type of socket, protocol*/
    sockfd=socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr,sizeof(servaddr));
    
    /*Print USAGE if ran without Server Port No.*/
    if(argc < 2 ){fprintf(stderr, "usage: %s <Port Number>\n", argv[0]); exit(1);}
    int svrport = atoi(argv[1]);
    
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(svrport); // Server port number
 
    /* Convert IPv4 and IPv6 addresses from text to binary form */
	inet_pton(AF_INET,"129.120.151.94",&(servaddr.sin_addr));
 
    printf("CLIENT\n");
    fprintf(fp, "CLIENT\n");
    
    /* Connect to the server */
    connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
    
    printf("\n--HANDSHAKE--\n");
    fprintf(fp, "\n--HANDSHAKE--\n");
    
    /*HANDSHAKE*/
    //create initial handshake tcp segment
    tcp_seg.src = svrport;
    tcp_seg.des = svrport;
    tcp_seg.seq = seqnum;
    tcp_seg.ack = 0;
    tcp_seg.hdr_flags = headerlength;
    tcp_seg.hdr_flags = tcp_seg.hdr_flags | SYN;
    tcp_seg.rec = 0;
    tcp_seg.cksum = 0;
    tcp_seg.ptr = 0;
    tcp_seg.opt = 0;
    calculateCksum(&tcp_seg);
    strcpy(tcp_seg.data, "");
    //send handshake segment
    showsnt(tcp_seg, fp);
    n = send(sockfd, &tcp_seg, sizeof(tcp_seg),0);
    
    //recieve connection granted segment
    n=recv(sockfd, &con_grant, sizeof(con_grant), 0);
    showrcv(con_grant, fp);
    
    //create tcp ackknowledgment segment
    ack_seg.src = svrport;
    ack_seg.des = svrport;
    ack_seg.seq = ++seqnum;
    ack_seg.ack = con_grant.seq + 1;
    ack_seg.hdr_flags = headerlength;
    ack_seg.hdr_flags = ack_seg.hdr_flags | SYN;
    ack_seg.hdr_flags = ack_seg.hdr_flags | ACK;
    ack_seg.rec = 0;
    ack_seg.cksum = 0;
    ack_seg.ptr = 0;
    ack_seg.opt = 0;
    calculateCksum(&ack_seg);
    strcpy(ack_seg.data, "");
    //send tcp acknowledgement segment
    showsnt(ack_seg, fp);
    n = send(sockfd, &ack_seg, sizeof(ack_seg), 0);
    
    /*SEND DATA*/
    printf("\n--DATA--\n");
    fprintf(fp, "\n--DATA--\n");
    
    /*Read the file being transfered into buffer*/
    readTransferFile(buffer);
    //printf("BUFFER: %s\n\n", buffer);
    data_ack = con_grant;
    
    //loop till all file segments sent
    for (int i =0; i < (size/d); i++){
        //send data
        snd_data.src = svrport;
        snd_data.des = svrport;
        snd_data.seq = data_ack.ack;
        snd_data.ack = data_ack.seq + 1;
        snd_data.hdr_flags = headerlength;
        snd_data.hdr_flags = snd_data.hdr_flags | SYN;
        snd_data.hdr_flags = snd_data.hdr_flags | ACK;
        snd_data.rec = 0;
        snd_data.cksum = 0;
        snd_data.ptr = 0;
        snd_data.opt = 0;
        calculateCksum(&snd_data);
        //copy data for sending
        strncpy(snd_data.data, (buffer + i*d), d);
        snd_data.data[d] = '\0';
        //send tcp acknowledgement segment
        showsnt(snd_data, fp);
        n = send(sockfd, &snd_data, sizeof(snd_data), 0);

        //recieve connection granted segment
        n=recv(sockfd, &data_ack, sizeof(data_ack), 0);
        showrcv(data_ack, fp);
    }
    
    printf("\n--CLOSE--\n");
    fprintf(fp, "\n--CLOSE--\n");
    
    /*CLOSE*/
    seqnum = rand();
    //create tcp close segment
    cls_cli.src = svrport;
    cls_cli.des = svrport;
    cls_cli.seq = seqnum;
    cls_cli.ack = 0;
    cls_cli.hdr_flags = headerlength;
    cls_cli.hdr_flags = cls_cli.hdr_flags | FIN;
    cls_cli.rec = 0;
    cls_cli.cksum = 0;
    cls_cli.ptr = 0;
    cls_cli.opt = 0;
    calculateCksum(&cls_cli);
    strcpy(cls_cli.data, "");
    //send the close segment
    showsnt(cls_cli, fp);
    send(sockfd, &cls_cli, sizeof(cls_cli), 0);
    
    //recieve acknowledgement
    recv(sockfd, &ack_cls, sizeof(ack_cls), MSG_WAITALL);
    showrcv(ack_cls, fp);
    
    //recieve server-side close request
    recv(sockfd, &cls_svr, sizeof(cls_svr), MSG_WAITALL);
    showrcv(cls_svr, fp);
    
    //create final acknowledgement
    final_ack.src = svrport;
    final_ack.des = svrport;
    final_ack.seq = ++seqnum;
    final_ack.ack = cls_svr.seq + 1;
    final_ack.hdr_flags = headerlength;
    final_ack.hdr_flags = final_ack.hdr_flags | ACK;
    final_ack.rec = 0;
    final_ack.cksum = 0;
    final_ack.ptr = 0;
    final_ack.opt = 0;
    calculateCksum(&final_ack);
    strcpy(final_ack.data, "");
    //send the final acknowledgement
    showsnt(final_ack, fp);
    send(sockfd, &final_ack, sizeof(final_ack), 0);
    
    fclose(fp);
    close(sockfd);  //close the connection
    
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

void readTransferFile(char buff[size]){
    FILE *fp;
    fp = fopen("data.txt", "r");
    
    if(fp){
        char c;
        int i = 0;
        
        /*read each char in file into buffer*/
        while ((c = fgetc(fp))!=EOF && i < size){
            buff[i] = c;
            i++;
        }
        buff[i] = '\0';
        
    } else{
        printf("ERROR: File <data.txt> not found.\n");
    }
    
    fclose(fp);
}

