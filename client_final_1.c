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

//#define FILESIZE 4914851  
char gl_packet[1500];

struct custom_header{
 uint32_t seq_no;
 uint16_t s_id;
 uint16_t d_id;
};

void callback(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char *packet) {
 memcpy(gl_packet,packet,1500);
}

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
    char datagram[5016] , source_ip[2],destination_ip[2], *data , *pseudogram,ack_nack,*nack;

    pcap_t *fp;
    char errbuf[PCAP_ERRBUF_SIZE];

    //struct sockaddr addr;
    //socklen_t fromlen;
    struct custom_header* ch = (struct  custom_header*) datagram;
    //struct sockaddr_in sin;
    //zero out the packet buffer
    struct timeval time_out;
    time_out.tv_sec=0;
    time_out.tv_usec=350000;
    //fromlen=sizeof(addr);
    
    if ( (fp= pcap_open_live("eth5",1500,1,1000,errbuf) ) == NULL)
    {
        fprintf(stderr,"\nUnable to open the adapter. is not supported by WinPcap\n");
        return 1;
    }


    
    /*

    int s = socket (AF_INET, SOCK_RAW, IPPROTO_RAW);

    if(s == -1)
    {
        perror("Failed to create socket");
        exit(1);
    }
    */
    
    memset(datagram,0,5016);
    /*
  if(recvfrom(s,datagram,65535,0,&addr,&fromlen)<0)
       perror("Receive failed");
    else
      {//printf("\n Received Success in file size ");
}
 */
     pcap_loop(fp,1, callback, NULL);

     memcpy(datagram,gl_packet,1500);
     memset(gl_packet,'\0',1500);    

    memcpy(&recvd_seq,datagram,4);
    recvd_seq=ntohl(recvd_seq);
    if (recvd_seq==0)
    {
      memcpy(&filesize,datagram+sizeof(struct custom_header),4);
      filesize=ntohl(filesize);
//      printf("\n Filesize got from server : %d",filesize);
    }
    else
    {
        //printf("\n File size not received ");
        exit(1);
    }

    memset(datagram,0,5016);
    strcpy(source_ip , argv[2]);
    strcpy(destination_ip , argv[3]);

    //sin.sin_family = AF_INET;
    //sin.sin_port = htons(80);
    //sin.sin_addr.s_addr = inet_addr (argv[3]);

    //Fill in the IP Header
    //iph->ihl = 5;
    //iph->version = 4;
    //iph->tos = 0;
    //iph->tot_len = sizeof (struct iphdr) + 5;
    //iph->id = htonl (54321); //Id of this packet
    //iph->frag_off = 0;
    //iph->ttl = 255;
    //iph->protocol = IPPROTO_RAW;
    //iph->check = 0;      //Set to 0 before calculating checksum
    ///iph->saddr = inet_addr ( source_ip );    //Spoof the source ip address
    //iph->daddr = sin.sin_addr.s_addr;

    //Ip checksum

    data=datagram+sizeof (struct custom_header);
    *data='0';
    data++;
    recvd_seq=htonl(recvd_seq);
    memcpy(data,&recvd_seq,4);
    //iph->check = csum ((unsigned short *) datagram, iph->tot_len);


    /*** Sending ACK for file size ***/

    /*
    if (sendto (s, datagram, iph->tot_len ,  0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
    {
        perror("sendto failed");
    }
     */
    if (pcap_sendpacket(fp,datagram,1500 /* size */) != 0)
    {
        //fprintf(stderr,"\nError sending the packet: \n", pcap_geterr(fp));
        pcap_perror(fp,"Send failed");
        return 1;
    }
    //    printf ("Packet Sending ACK. Length & Sequence: %d \n" , iph->tot_len);

    int i=filesize/1491;
    int num_of_packets = i;
    int last_packet_size=filesize%1491;
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
    //printf("\n Assigning temp data");
    temp_data=mmap(NULL,(filesize),PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
    
    if(temp_data==MAP_FAILED)
    {
        perror("MMAP failed");
        exit(1);
    }

    recvdata=temp_data;
    /*
   if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,&time_out,sizeof(time_out)) < 0)
    {
      perror("Could not set timer");
    }
    */
    /*** Receiving data from the sender ***/
    while (i>=0)
    {
      //printf("\n Recvd : %d",i);
        /*
        if(recvfrom(s,datagram,65535,0,&addr,&fromlen)<0)
        {
            //perror("Error reading 1");
            break;
        } 
       */
     pcap_loop(fp,1, callback, NULL);
     memcpy(datagram,gl_packet,1500);
     memset(gl_packet,'\0',1500);    

    memcpy(&recvd_seq,datagram,4);
    recvd_seq=ntohl(recvd_seq);
    if (recvd_seq==0){
        /*** Sending ACK again for file size ***/
     /*   if (sendto (s, datagram, iph->tot_len ,  0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
        {
            perror("sendto failed");
        }
        else
        {
            printf ("Packet Send. Length & Sequence: %d \n" , iph->tot_len);
        }*/
        continue;
   }

    //if (i!=(filesize/1475)) {
        nack[recvd_seq]= '1';
    //}
   // printf("\n Entered While : %d, i = %d",recvd_seq,i);

    //Data part
    data = datagram + sizeof(struct custom_header);//+ sizeof(struct tcphdr)
    if(recvd_seq >filesize)
       recvd_seq=1;
    memcpy((temp_data + ((recvd_seq-1) * 1491)),data, recvd_seq == (num_of_packets+1)? last_packet_size : 1491);
   // printf("\n Done memcopy for seq number : %d ",recvd_seq);
    //printf("\n i : %d",i);
    //temp_data=temp_data+strlen(data);
    i--;
    }
  //printf("\n Packets received and checking for holes filesize received \n");
/* Check to be removed */

    int k,flag=1,m;
    uint16_t i_c=0;
        //exit(1);


/* Sending NACK's */


    //Ip checksum

    data=datagram+sizeof (struct custom_header);
    m=0;
    do
    {
    memset(datagram, 0 , 8);
    strcpy(source_ip , argv[2]);
    strcpy(destination_ip, argv[3]);
    memcpy(data,source_ip,sizeof(source_ip));
    data=data+2;
    memcpy(data,destination_ip,sizeof(destination_ip));
    data=data+2;
    //sin.sin_family = AF_INET;
    //sin.sin_port = htons(80);
    //sin.sin_addr.s_addr = inet_addr (argv[3]);
 
    //iph=(struct iphdr*)datagram;
    //Fill in the IP Header
    //iph->ihl = 5;
    //iph->version = 4;
    //iph->tos = 0;
    //iph->tot_len = sizeof (struct iphdr) + 1;
    //iph->id = htonl (54321); //Id of this packet
    //iph->frag_off = 0;
    //iph->ttl = 255;
    //iph->protocol = IPPROTO_RAW;
    //iph->check = 0;      //Set to 0 before calculating checksum
    ///iph->saddr = inet_addr ( source_ip );    //Spoof the source ip address
    //iph->daddr = sin.sin_addr.s_addr;
    data=datagram+sizeof(struct custom_header);
    memset(data,0,1492);
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
        //    printf("\n NACKed packets still available");
            *data='\0';
            //tot_len=sizeof(struct iphdr)+1+(i_c*4)+2;
            i_c=htons(i_c);
            memcpy((datagram+sizeof(struct custom_header)+1),&i_c,2);
            
            //iph->tot_len=htons(tot_len);
            //printf("\n Nack pack number : %d data_len=%d ,i_c in htons = %d\n",m, tot_len,i_c);
            //iph->check = csum ((unsigned short *) datagram, tot_len); // Print the seq number and see for cross verification
                    int i = 0;
                /*    for(i = 0;i < 100; i+=10) {
                        printf("%0.2x %0.2x %0.2x %0.2x %0.2x %0.2x %0.2x %0.2x %0.2x %0.2x \n",
                                datagram[i], datagram[i+1], datagram[i+2], datagram[i+3],  datagram[i+4],
                                datagram[i+5], datagram[i+6], datagram[i+7], datagram[i+8],  datagram[i+9]);
                    }*/

            /*if (sendto (s, datagram, tot_len ,  0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
            {
           // perror("sendto failed");
            }
            */
           if (pcap_sendpacket(fp,datagram,1500 /* size */) != 0)
          {
        //fprintf(stderr,"\nError sending the packet: \n", pcap_geterr(fp));
        pcap_perror(fp,"Send failed");
        return 1;
           }         
           else
           {
            m++;
           // printf ("Packet Send. Length & Sequence: %d \n" , m);
            //printf("\n Nack pack number : %d data_len=%d",m, tot_len);
            data = datagram + sizeof(struct custom_header);
            memset(data,0,1480);
            *data='1';
            data+=3;
            i_c=0;
           }
        }
     }
    //printf("\n Outside for loop :");
    //printf("\n Waiting for lost packets ");
    while(flag)  /* NACK is a integer array pointer*/
   {
//        memset(datagram, 0, 5016);
        //if(recvfrom(s,datagram,65535,0,&addr,&fromlen)<0)
         char temp[1500];

         pcap_loop(fp,1, callback, NULL);
         memcpy(temp,gl_packet,1500);
         memset(gl_packet,'\0',1500);

         
         if(temp==NULL)
        { //  Receiving the lost packets
         //perror("Receive failed 1");
        break;
        }
        else
        {
          //  printf("\n Received Datagram");
            memcpy(&lost_seq,datagram+sizeof(struct custom_header),4);
            lost_seq=ntohl(lost_seq);
            nack[lost_seq]= '1';
            int pack_size=(lost_seq==num_of_packets+1)? last_packet_size : 1491;
           // printf("\n Received  the lost packet %d, size %d",lost_seq,pack_size);
            memcpy((temp_data+((lost_seq-1)*(1491))),datagram+sizeof(struct custom_header),lost_seq == (num_of_packets+1)? last_packet_size :
 1491);
          //  printf("\n Memcopy successfll");
        }
             /* Copying the data into the buffer.*/
     }
  // printf("\n Done receiving lost packets");
   }while(flag==1);

free(nack);
k=0;

/*  Sending ACK for entire file */

    data = datagram + sizeof(struct custom_header);
    memset(data,0,1492);

    *data='0';
    //iph->tot_len= sizeof(struct iphdr)+1;
    //iph->check = csum ((unsigned short *) datagram, iph->tot_len);
    while(k<=3)
    {
    k++;
    /*
    if (sendto (s, datagram, iph->tot_len ,  0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
        {
           // perror("sendto failed");
        }
        */
    if (pcap_sendpacket(fp,datagram,1500 /* size */) != 0)
          {
        //fprintf(stderr,"\nError sending the packet: \n", pcap_geterr(fp));
        pcap_perror(fp,"Send failed");
        return 1;
           } 
    else
        {
          //  printf ("Packet Send. Length & Sequence: %d \n" , iph->tot_len);
        }
    }

/*** Write into file and close ***/

    struct timeval time1;
    gettimeofday(&time1,NULL);
    //printf("\n end time %d : %d\n",(int)time1.tv_sec,(int)time1.tv_usec);
    //fwrite(recvdata,1,filesize,newfile);
    fclose(newfile) ;
    //close(s);
    munmap(recvdata,filesize);
    return 0;
}
