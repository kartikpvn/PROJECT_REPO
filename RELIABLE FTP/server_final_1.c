#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#include <errno.h>
#include <netdb.h>
#include <linux/if_packet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/ether.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <pcap/pcap.h>
#include <net/if.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/mman.h>


char gl_packet[1500];

struct custom_header{
 uint32_t seq_no;
 uint16_t s_id;
 uint16_t d_id;
};


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

void callback(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char *packet) {
 memcpy(gl_packet,packet,1500);
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
    // 4 bytes of sequence
    uint32_t seq=0,seq_sent;
    //struct sockaddr addr;   
    pcap_t *fp;
    char errbuf[PCAP_ERRBUF_SIZE];

    //socklen_t fromlen;
    // 2 bytes of source and destination Index number
    char datagram[1500] , source_ip[2],destination_ip[2] , *data , ack_pack[1500],datagram1[1500];
    struct timeval t1,t2,time_out;
    //fromlen=sizeof(addr);
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
 //   printf("\n Length of temp_data : %d",strlen(filedata));
    // 1500 - 1(null character) - 8(sizeof(struct custom_hdr))=1491
    counter=filesize/1491;
    remaining=filesize%1491;
    int num_of_packets=counter,pack_size;
    //int s = socket (AF_INET, SOCK_RAW, IPPROTO_RAW);
    // opening the pcap handle 
     if ( (fp= pcap_open_live("eth4",1500,1,1000,errbuf) ) == NULL)
    {
        fprintf(stderr,"\nUnable to open the adapter. is not supported by WinPcap\n");
        return 1;
    }

    /*
    if(s == -1)
    {
        perror("Failed to create socket");
        exit(1);
    }
    */

    time_out.tv_sec=0;
    time_out.tv_usec=300000;
    /*if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,&time_out,sizeof(time_out)) < 0)y                                              dwwwwww2
    {
      perror("Could not set timer");
    }*/


    memset (datagram, 0, 1500);
    memset (datagram1, 0, 1500);


    strcpy(source_ip , argv[2]);
    strcpy(destination_ip , argv[3]);

    struct custom_header* ch;

     ch = (struct custom_header*) datagram;
     seq_sent=htonl(seq);
     // filling the sequence number
     // filling the source IP.
     // filling the destination IP in the custom header.
     ch->seq_no=seq_sent;
     ch->s_id= (uint16_t) atoi(source_ip);
     ch->d_id=(uint16_t) atoi(destination_ip);

    //struct sockaddr_in sin;

    //Data part
    data = datagram + sizeof(struct custom_header) ;
    

    
    file_sent=htonl(filesize);
    //printf("\n File size : %d",filesize);
    //memcpy(datagram,&seq_sent,sizeof(uint32_t));
    //printf("\n Sequence number length: %d",strlen(data));
    memcpy(data,&file_sent,sizeof(uint32_t));
    //printf("\n filesize length : %d",strlen(data));

    //some address resolution
    //sin.sin_family = AF_INET;
    //sin.sin_port = htons(80);
    
    //sin.sin_addr.s_addr = inet_addr (argv[3]);

    //Fill in the IP Header
    //iph->ihl = 5;
    //iph->version = 4;
    //iph->tos = 0;
    //iph->tot_len = sizeof (struct iphdr) + 8;
    //iph->id = htonl (54321); //Id of this packet
    //iph->frag_off = 0;
    //iph->ttl = 255;
    //iph->protocol = IPPROTO_RAW;
    //iph->check = 0;      //Set to 0 before calculating checksum
    //iph->saddr = inet_addr ( source_ip );    //Spoof the source ip address
    //iph->daddr = sin.sin_addr.s_addr;

    //Ip checksum
    //iph->check = csum ((unsigned short *) datagram, iph->tot_len);

    //while(ack_recvd!=1)  *** commented originally ***
    {
     //printf("\n In while \n");

     /*
     if (sendto (s, datagram, iph->tot_len ,  0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
        {
            perror("sendto failed");
        }
        */
//           printf ("Packet Send. Length & Sequence: %d %d\n" , iph->tot_len,seq);
     
     
     /*
      if(recvfrom(s,ack_pack,65535,0,&addr,&fromlen)<0)
                perror("\n Receive failed");
      else
         {if (*(ack_pack+sizeof(struct iphdr))=='0')
                  {
                    //printf("\n ACK received ");
                  ack_recvd=1;
                }
         }
    }
     */
     if (pcap_sendpacket(fp,datagram,1500 /* size */) != 0)
    {
        //fprintf(stderr,"\nError sending the packet: \n", pcap_geterr(fp));
        pcap_perror(fp,"Send failed");
        return 1;
    }
   
	pcap_loop(fp,1, callback, NULL);
	memcpy(ack_pack,gl_packet,1500);
    memset(gl_packet,'\0',1500);

     if(ack_pack==NULL)
                perror("\n Receive failed");
      else
         {if (*(ack_pack+sizeof(struct custom_header))=='0')
                  {
                    //printf("\n ACK received ");
                  ack_recvd=1;
                }
         }

    }

    char buf[5000];
    int i=0;
    gettimeofday(&t1,NULL);

    memset(buf, 0, 5000);
    begin_time = timeval();
    printf(" Data Transfer Started \n ");


    while (counter >= 0)
    {

        if(counter!=0){
            memcpy(buf,temp_data,1491);
            pack_size=1491;
            buf[1492]='\0';
        } else {
            if (remaining!=0)
              {memcpy(buf,temp_data,remaining);
              buf[remaining+1]='\0';
              pack_size=remaining;}
            else
              break;}
        data=datagram;
        seq++;
        //printf("\n %d : ",seq);
        seq_sent=htonl(seq);
        memcpy(data,&seq_sent,sizeof(uint32_t));
        data=data+4;
        memcpy (data,source_ip,sizeof(source_ip));
        data=data+2;
        memcpy (data,destination_ip,sizeof(destination_ip));
        //printf("\n data : %d", strlen(data));
        //printf("Entered WHile ***************************************\n");

        //iph->tot_len = sizeof (struct iphdr) + pack_size+4;
        //iph->check = csum ((unsigned short *) datagram, iph->tot_len);
        //if(i%120==0)
         //  usleep(1500);
        /*
        if (sendto (s, datagram, iph->tot_len ,  0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
        {
            perror("sendto failed");
        }
        else
        {
            //printf ("Packet Send. Length & Sequence: %d %d\n" , iph->tot_len,seq); 
        }
        */
        if (pcap_sendpacket(fp,datagram,1500 /* size */) != 0)
    {
        //fprintf(stderr,"\nError sending the packet: \n", pcap_geterr(fp));
        pcap_perror(fp,"Send failed");
        return 1;
    } 
       
        memset (buf, 0, 5000);
#if 1
        //printf("\n Inside If 0");
        temp_data=temp_data+1491;
        counter--;
#endif
#if 0 
        //printf("\n Inside If 1");    
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

  /*if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,&time_out,sizeof(time_out)) < 0)
    {
      perror("Could not set timer");
    }*/

    int k=0,m=0;
    while(1)
    {
            //memset(datagram,0,1500);
            k=0;
            lost_track=lost_seq;
            while(1)
            {
              //      memset(datagram,0,1500);
                    //data=datagram+sizeof(struct iphdr);
                    //printf("\n Waiting for ACK or NACK \n");
            	    char temp[1500];
            	    pcap_loop(fp,1, callback, NULL);
            	    memcpy(temp,gl_packet,1500);
                    memset(gl_packet,'\0',1500);
                    
            	    if(temp==NULL)
                        perror("\n Receive failed");
                    else {
                    	memcpy(datagram1,temp,1500);
                    }
                   /*
                    if(recvfrom(s,datagram1,1500,0,&addr,&fromlen)<0)	
                    {
                            // *data='1';
                           perror(" Recvfrom failed");
                            break;
                    }
                    */
                    int i = 0;
                    //printf("\n About to print datagram");
                    /*for(i = 0;i < 100; i+=10) {
                        printf("%0.2x %0.2x %0.2x %0.2x %0.2x %0.2x %0.2x %0.2x %0.2x %0.2x \n",
                                datagram1[i], datagram1[i+1], datagram1[i+2], datagram1[i+3],  datagram1[i+4],
                                datagram1[i+5], datagram1[i+6], datagram1[i+7], datagram1[i+8],  datagram1[i+9]);
                    }*/
                   data = datagram1+sizeof(struct custom_header);
                    //iph =(struct iphdr*)datagram1;
                    //tot_len=iph->tot_len;
                    //memcpy(&tot_len,datagram+2,2);
                    //tot_len = ntohs(tot_len);
                    
                    //tot_len = tot_len - sizeof(struct iphdr);

                    if(*data=='0')
                            break;
                    else
                    {
                            memcpy(&nackd_number,(datagram1+sizeof(struct custom_header)+1),2);
                            nackd_number=ntohs(nackd_number);
                            k++;
                            //printf("\n Received NACK : %d string_len %d , also %d\n", k, nackd_number, tot_len);
                            memcpy(lost_track,datagram1 + sizeof(struct custom_header)+3,(nackd_number*4));
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
                    //memcpy(datagram, iph,sizeof(struct iphdr));
                    //iph=(struct iphdr*)datagram;
                    strcpy(source_ip , argv[2]);
                    strcpy(destination_ip,argv[3]);
                    memcpy(datagram+4,source_ip,sizeof(source_ip));
                    memcpy(datagram+6,destination_ip,sizeof(destination_ip));
                    //sin.sin_family = AF_INET;
                    //sin.sin_port = htons(80);
                    //sin.sin_addr.s_addr = inet_addr (argv[3]);
                    //Fill in the IP Header
                   //iph->ihl = 5;
                    //iph->version = 4;
                    //iph->tos = 0;
                    //iph->tot_len = sizeof (struct iphdr) + 8;
                    //iph->id = htonl (54321); //Id of this packet
                    //iph->frag_off = 0;
                    //iph->ttl = 255;
                    //iph->protocol = IPPROTO_RAW;
                    //iph->check = 0;      //Set to 0 before calculating checksum
                    //iph->saddr = inet_addr ( source_ip );    //Spoof the source ip address
                    //iph->daddr = sin.sin_addr.s_addr;
                    //printf("\n Inside Else : %d ",strlen(lost_seq));
                    lost_track=lost_seq;
                    //printf("\n Number of sequence numbers %d",no_seq_num);
                    for(m=0;m < no_seq_num;m++)
                    {
                            //printf("\n Inside For");
                            data = datagram;
                            memset(data,0,1492);
                            memcpy(&recvd_seq,lost_track,4);
                            memcpy(data,&recvd_seq,4);
                            lost_track=lost_track+4;
                            data=data+8;
                            recvd_seq=ntohl(recvd_seq);
                            //printf("Recvd_seq : %d, %d, %d \n",recvd_seq,m,no_seq_num);
                            temp_data=filedata+((recvd_seq-1)*1491);
                            //printf("\n Temp data offset done \n");
                            pack_size=(recvd_seq==num_of_packets+1) ? remaining : 1491;
                            //printf("Packet size = %d \n",pack_size);
#if 1
                            memcpy(buf,temp_data,recvd_seq==num_of_packets+1 ? remaining:1491);       // Vandhana instead of 1475 check if it is last packet and then add only remaining ones
                            buf[1492]='\0';
                            memcpy(data,buf,pack_size);
                            //iph->tot_len=sizeof(struct iphdr)+4+pack_size;
                        //iph->check = csum ((unsigned short *) datagram, iph->tot_len);
                            /*
                            if (sendto (s, datagram, iph->tot_len ,  0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
                            {
                                    perror("sendto failed 2");
                            }
                            else
                            {
                                    //printf ("Packet Send. Length & Sequence for NACKs: %d \n" , iph->tot_len,recvd_seq);
                            }
                            */
#endif
                          if (pcap_sendpacket(fp,datagram,1500 /* size */) != 0)
                             {
                            //fprintf(stderr,"\nError sending the packet: \n", pcap_geterr(fp));
                            pcap_perror(fp,"Send failed");
                            return 1;
                            }  
                    }
                    //printf("\n Processed received NACK packets");
                    memset(lost_seq,0,3000000);
                    no_seq_num=0;
            }
    }

    pcap_close(fp);
    munmap(filedata,filesize);
    free(lost_seq);
    //printf(" \n Start time : %d : %d \n",t1.tv_sec,t1.tv_usec);
    return 0;
}
