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
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <unistd.h>

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
long timeval()
{
    struct timeval times;
    gettimeofday(&times, NULL);
    return (times.tv_sec*1000000)+times.tv_usec;
}



int main (int argc, char *argv[])
{
    //Create a raw socket
    long begin_time;
    long time1;
    long time2;

    FILE *myfile;
    uint32_t filesize,file_sent;
    struct stat stat1;
    void *filedata,*temp_data;
    int fd,counter,remaining,ack_recvd=0;
    uint32_t seq=0,seq_sent;
    struct sockaddr addr;

    socklen_t fromlen;
    char datagram[1500] , source_ip[32] , *data , ack_pack[1500],datagram1[1500];
    struct timeval t1,t2,time_out;

    fromlen=sizeof(addr);
    stat(argv[1],&stat1);
    filesize=stat1.st_size;
    myfile = fopen(argv[1],"r");
    fd=fileno(myfile);

    if ( myfile == NULL)
    {
        printf("Cannot open file \n");
        exit(1);
    }
    filedata=mmap(NULL,filesize,PROT_READ,MAP_PRIVATE | MAP_POPULATE,fd,0);
    temp_data=filedata;
    counter=filesize/1475;
    remaining=filesize%1475;
    int num_of_packets=counter,pack_size;

    int s = socket (AF_INET, SOCK_RAW, IPPROTO_RAW);

    if(s == -1)
    {
        perror("Failed to create socket");
        exit(1);
    }

    time_out.tv_sec=0;
    time_out.tv_usec=300000;
 
    memset (datagram, 0, 1500);
    memset (datagram1, 0, 1500);

    //IP header
    struct iphdr *iph = (struct iphdr *) datagram;


    struct sockaddr_in sin;

    //Data part
    data = datagram + sizeof(struct iphdr) ;
    seq_sent=htonl(seq);


    file_sent=htonl(filesize);
    memcpy(data,&seq_sent,sizeof(uint32_t));
    data=data+4;
    memcpy(data,&file_sent,sizeof(uint32_t));
 
    //some address resolution
    strcpy(source_ip , argv[2]);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr (argv[3]);

    //Fill in the IP Header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof (struct iphdr) + 8;
    iph->id = htonl (54321); //Id of this packet
    iph->frag_off = 0;
    iph->ttl = 255;
    iph->protocol = IPPROTO_RAW;
    iph->check = 0;      //Set to 0 before calculating checksum
    iph->saddr = inet_addr ( source_ip );    //Spoof the source ip address
    iph->daddr = sin.sin_addr.s_addr;

    //Ip checksum
    iph->check = csum ((unsigned short *) datagram, iph->tot_len);

    
     if (sendto (s, datagram, iph->tot_len ,  0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
        {
            perror("sendto failed");
        }
      if(recvfrom(s,ack_pack,65535,0,&addr,&fromlen)<0)
                perror("\n Receive failed");
      else
         {if (*(ack_pack+sizeof(struct iphdr))=='0')
                  {
                    //printf("\n ACK received ");
                  ack_recvd=1;
                }
         }
    

    char buf[5000];
    int i=0;
    gettimeofday(&t1,NULL);
    memset (buf, 0, 5000);
    begin_time = timeval();
    printf(" Data Transfer Started \n ");


    while (counter >= 0)
    {

        if(counter!=0){
            memcpy(buf,temp_data,1475);
            pack_size=1475;
            buf[1476]='\0';
        } else {
            if (remaining!=0)
              {memcpy(buf,temp_data,remaining);
              buf[remaining+1]='\0';
              pack_size=remaining;}
            else
              break;}
        data=datagram+sizeof(struct iphdr);
        seq++;
        seq_sent=htonl(seq);
        memcpy(data,&seq_sent,sizeof(uint32_t));
        data=data+4;
        memcpy (data, buf,pack_size);
    
        iph->tot_len = sizeof (struct iphdr) + pack_size+4;
        iph->check = csum ((unsigned short *) datagram, iph->tot_len);
        if (sendto (s, datagram, iph->tot_len ,  0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
        {
            perror("sendto failed");
        }

        memset (buf, 0, 5000);
#if 1
        temp_data=temp_data+1475;
        counter--;
#endif
#if 0 
        temp_data=temp_data+1475*2;
        counter-=2;
        seq=seq+1; 
#endif
        //i++;
    }
    temp_data= filedata;


/*** Receiving NACK's or ACK and processing them***/

    char *lost_seq,*lost_track;
    uint32_t recvd_seq,dummy;
    uint16_t tot_len,nackd_number;
    int no_seq_num=0;
    //struct iphdr *iph;
    lost_seq=(char*)malloc(3000000);
    memset(lost_seq,0,3000000);
    lost_track=lost_seq;


    int k=0,m=0;
    while(1)
    {
            //memset(datagram,0,1500);
            k=0;
            lost_track=lost_seq;
            while(1)
            {
                    if(recvfrom(s,datagram1,1500,0,&addr,&fromlen)<0)
                    {
                            // *data='1';
                           perror(" Recvfrom failed");
                            break;
                    }
                    int i = 0;
                   data = datagram1+sizeof(struct iphdr);
                    iph =(struct iphdr*)datagram1;
                    tot_len=iph->tot_len;
                    tot_len = ntohs(tot_len);
                    
                    tot_len = tot_len - sizeof(struct iphdr);

                    if(*data=='0')
                            break;
                    else
                    {
                            memcpy(&nackd_number,(datagram1+sizeof(struct iphdr)+1),2);
                            nackd_number=ntohs(nackd_number);
                            k++;
                            memcpy(lost_track,datagram1 + sizeof(struct iphdr)+3,(nackd_number*4));
                            lost_track=lost_track + (nackd_number*4);
                            no_seq_num=no_seq_num+nackd_number;
                            if(nackd_number<367)
							    break;
                    }

            }
            *(lost_track+1)='\0';
            if(*data=='0')
                    break;
            else
            {
                    memset(datagram,0,1500);
                    iph=(struct iphdr*)datagram;
                    strcpy(source_ip , argv[2]);
                    sin.sin_family = AF_INET;
                    sin.sin_addr.s_addr = inet_addr (argv[3]);

                    //Fill in the IP Header
                   iph->ihl = 5;
                    iph->version = 4;
                    iph->tos = 0;
                    iph->tot_len = sizeof (struct iphdr) + 8;
                    iph->id = htonl (54321); //Id of this packet
                    iph->frag_off = 0;
                    iph->ttl = 255;
                    iph->protocol = IPPROTO_RAW;
                    iph->check = 0;      //Set to 0 before calculating checksum
                    iph->saddr = inet_addr ( source_ip );    //Spoof the source ip address
                    iph->daddr = sin.sin_addr.s_addr;
                    lost_track=lost_seq;
                    for(m=0;m < no_seq_num;m++)
                    {
                            data = datagram + sizeof(struct iphdr);
                            memset(data,0,1480);
                            memcpy(&recvd_seq,lost_track,4);
                            memcpy(data,&recvd_seq,4);
                            lost_track=lost_track+4;
                            data=data+4;
                            recvd_seq=ntohl(recvd_seq);
                            temp_data=filedata+((recvd_seq-1)*1475);
                            pack_size=(recvd_seq==num_of_packets+1) ? remaining : 1475;
#if 1
                            memcpy(buf,temp_data,recvd_seq==num_of_packets+1 ? remaining:1475);       // Vandhana instead of 1475 check if it is last packet and then add only remaining ones
                            buf[1476]='\0';
                            memcpy(data,buf,pack_size);
                            iph->tot_len=sizeof(struct iphdr)+4+pack_size;
                        iph->check = csum ((unsigned short *) datagram, iph->tot_len);
                            if (sendto (s, datagram, iph->tot_len ,  0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
                            {
                                    perror("sendto failed 2");
                            }
                            else
                            {
                                    printf ("Packet Send. Length & Sequence for NACKs: %d \n" , iph->tot_len,recvd_seq);
                            }
#endif
                    }
                    memset(lost_seq,0,3000000);
                    no_seq_num=0;
            }
    }

    close(s);
    munmap(filedata,filesize);
    free(lost_seq);
    return 0;
}
