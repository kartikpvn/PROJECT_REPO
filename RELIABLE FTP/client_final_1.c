#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <assert.h>
#include<sys/time.h>
#include <arpa/inet.h>

//#define FILESIZE 4914851  
long timeval()
{
    struct timeval times;
    gettimeofday(&times, NULL);
    return (times.tv_sec*1000000)+times.tv_usec; 
}

unsigned short csum(unsigned short *ptr,int nbytes) 
{
    register long sum;
    unsigned short oddbyte;
    register short answer;
 
    sum=0;
    while(nbytes>1) {
        sum+=*ptr++;
        nbytes-=2;
    }
    if(nbytes==1) {
        oddbyte=0;
        *((u_char*)&oddbyte)=*(u_char*)ptr;
        sum+=oddbyte;
    }
 
    sum = (sum>>16)+(sum & 0xffff);
    sum = sum + (sum>>16);
    answer=(short)~sum;
     
    return(answer);
}
    
int main (int argc, char *argv[])
{   
    FILE *newfile;
    void *recvdata,*temp_data;
    int fd,recvd_seq;
    struct stat stat1;
    uint32_t filesize,seq_no,lost_seq;
    char datagram[5016] , source_ip[32] , *data , *pseudogram,ack_nack,*nack;
    struct sockaddr addr;
    socklen_t fromlen;
    struct iphdr *iph = (struct iphdr *) datagram;
    struct sockaddr_in sin;
    struct timeval time_out;
    time_out.tv_sec=0;
    time_out.tv_usec=350000;
    fromlen=sizeof(addr);

    int s = socket (AF_INET, SOCK_RAW, IPPROTO_RAW);

    if(s == -1)
    {
        perror("Failed to create socket");
        exit(1);
    }
    
    memset(datagram,0,5016);
  if(recvfrom(s,datagram,65535,0,&addr,&fromlen)<0)
       perror("Receive failed");

    memcpy(&recvd_seq,datagram+sizeof(struct iphdr),4);
    recvd_seq=ntohl(recvd_seq);
    if (recvd_seq==0)
    {
      memcpy(&filesize,datagram+sizeof(struct iphdr)+4,4);
      filesize=ntohl(filesize);
    }
    else
    {
        //printf("\n File size not received ");
        exit(1);
    }

    memset(datagram,0,5016);
    strcpy(source_ip , argv[2]);
    sin.sin_family = AF_INET;
    //sin.sin_port = htons(80);
    sin.sin_addr.s_addr = inet_addr (argv[3]);

    //Fill in the IP Header
    iph->ihl = 5;
iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof (struct iphdr) + 5;
    iph->id = htonl (54321); //Id of this packet
    iph->frag_off = 0;
    iph->ttl = 255;
    iph->protocol = IPPROTO_RAW;
    iph->check = 0;      //Set to 0 before calculating checksum
    iph->saddr = inet_addr ( source_ip );    //Spoof the source ip address
    iph->daddr = sin.sin_addr.s_addr;

    //Ip checksum

    data=datagram+sizeof (struct iphdr);
    *data='0';
    data++;
    recvd_seq=htonl(recvd_seq);
    memcpy(data,&recvd_seq,4);
    iph->check = csum ((unsigned short *) datagram, iph->tot_len);


    /*** Sending ACK for file size ***/

    if (sendto (s, datagram, iph->tot_len ,  0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
    {
        perror("sendto failed");
    }
  

    int i=filesize/1475;
    int num_of_packets = i;
    int last_packet_size=filesize%1475;
    printf("\n File receive started \n ");
    newfile= fopen(argv[1], "w+");

    if ( newfile == NULL)
    {
        printf("Cannot open file \n");
        exit(1);
    }

    nack=(char*)malloc(i*sizeof(char));
    memset(nack,'0',(i*sizeof(char)));

    fd=fileno(newfile);
    ftruncate(fd,filesize);
    
    temp_data=mmap(NULL,(filesize),PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
    if(temp_data==MAP_FAILED)
    {
        perror("MMAP failed");
        exit(1);
    }
    recvdata=temp_data;
  if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,&time_out,sizeof(time_out)) < 0)
    {
      perror("Could not set timer");
    }

    /*** Receiving data from the sender ***/
    while (i>=0)
    {
      
        if(recvfrom(s,datagram,65535,0,&addr,&fromlen)<0)
        {
            //perror("Error reading 1");
            break;
        }

    memcpy(&recvd_seq,datagram+sizeof(struct iphdr),4);
    recvd_seq=ntohl(recvd_seq);
    if (recvd_seq==0){
       
        continue;
   }

   
        nack[recvd_seq]= '1';
   
  

    //Data part
    data = datagram + sizeof(struct iphdr) + 4;//+ sizeof(struct tcphdr)
    if(recvd_seq >filesize)
       recvd_seq=1;
    memcpy((temp_data + ((recvd_seq-1) * 1475)),data, recvd_seq == (num_of_packets+1)? last_packet_size : 1475);
    i--;
    }
/* Check to be removed */

    int k,flag=1,m;
    uint16_t i_c=0;
        //exit(1);


/* Sending NACK's */


    //Ip checksum

    data=datagram+sizeof (struct iphdr);
    m=0;
    do
    {
    memset(datagram, 0 , 20);
    strcpy(source_ip , argv[2]);
    sin.sin_family = AF_INET;
    //sin.sin_port = htons(80);
    sin.sin_addr.s_addr = inet_addr (argv[3]);

    iph=(struct iphdr*)datagram;
    //Fill in the IP Header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof (struct iphdr) + 1;
    iph->id = htonl (54321); //Id of this packet
    iph->frag_off = 0;
    iph->ttl = 255;
   iph->protocol = IPPROTO_RAW;
    iph->check = 0;      //Set to 0 before calculating checksum
    iph->saddr = inet_addr ( source_ip );    //Spoof the source ip address
    iph->daddr = sin.sin_addr.s_addr;
    data=datagram+sizeof(struct iphdr);
    memset(data,0,1480);
    *data='1';
    data=data+3;
    flag= 0;
    uint16_t tot_len = 0;
    // printf("\n Entered into do while");
    for(k=1;k<=num_of_packets+1;k++)
     {
       if(nack[k]!= '1')
       {
        recvd_seq=htonl(k);
        memcpy(data,&recvd_seq,4);
        data=data+4;
        i_c++;
        flag=1;
       }

       if(i_c==367 || ((k==num_of_packets+1) && flag))
       {
            *data='\0';
            tot_len=sizeof(struct iphdr)+1+(i_c*4)+2;
            i_c=htons(i_c);
            memcpy((datagram+sizeof(struct iphdr)+1),&i_c,2);
            
            iph->tot_len=htons(tot_len);
            iph->check = csum ((unsigned short *) datagram, tot_len); // Print the seq number and see for cross verification
                    int i = 0;
            if (sendto (s, datagram, tot_len ,  0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
            {
            perror("sendto failed");
            }
           else
           {
            m++;
            data = datagram + sizeof(struct iphdr);
            memset(data,0,1480);
            *data='1';
            data+=3;
            i_c=0;
           }
        }
     }
    while(flag)  /* NACK is a integer array pointer*/
   {
        if(recvfrom(s,datagram,65535,0,&addr,&fromlen)<0){ //  Receiving the lost packets
        break;
        }
        else
        {
            memcpy(&lost_seq,datagram+sizeof(struct iphdr),4);
            lost_seq=ntohl(lost_seq);
            nack[lost_seq]= '1';
            int pack_size=(lost_seq==num_of_packets+1)? last_packet_size : 1475;
            memcpy((temp_data+((lost_seq-1)*(1475))),datagram+sizeof(struct iphdr)+4,lost_seq == (num_of_packets+1)? last_packet_size :
 1475);
        }
             /* Copying the data into the buffer.*/
     }
   }while(flag==1);

free(nack);
k=0;

/*  Sending ACK for entire file */

    data = datagram + sizeof(struct iphdr);
    memset(data,0,1480);

    *data='0';
    iph->tot_len= sizeof(struct iphdr)+1;
    iph->check = csum ((unsigned short *) datagram, iph->tot_len);
    while(k<=3)
    {
    k++;
    if (sendto (s, datagram, iph->tot_len ,  0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
        {
            perror("sendto failed");
          }
    }

/*** Write into file and close ***/

    struct timeval time1;
    gettimeofday(&time1,NULL);
    fclose(newfile) ;
    close(s);
    munmap(recvdata,filesize);
    return 0;
}
