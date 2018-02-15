#include <pcap/pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <unistd.h>

/* new checksum calculation fucntion */

char devices[10][10];
int num_devices;
struct in_addr myaddress[10];
pthread_mutex_t m;
struct bpf_program fp;
uint16_t icmp_checksum(uint16_t *buffer, uint32_t size);

uint16_t ip_checksum(void* vdata,size_t length) 
{
  // Cast the data pointer to one that can be indexed.
  char* data=(char*)vdata;
  // Initialise the accumulator.
  uint64_t acc=0xffff;
  // Handle any partial block at the start of the data.
  unsigned int offset=((uintptr_t)data)&3;
  if (offset) 
  {
    size_t count=4-offset;
    if (count>length) count=length;
    uint32_t word=0;
    memcpy(offset+(char*)&word,data,count);
    acc+=ntohl(word);
    data+=count;
    length-=count;
  }

  // Handle any complete 32-bit blocks.
  char* data_end=data+(length&~3);
  while (data!=data_end) {
    uint32_t word;
    memcpy(&word,data,4);
    acc+=ntohl(word);
    data+=4;
  }
  length&=3;

  // Handle any partial block at the end of the data.
  if (length) 
  {
    uint32_t word=0;
    memcpy(&word,data,length);
    acc+=ntohl(word);
  }

  // Handle deferred carries.
  acc=(acc&0xffffffff)+(acc>>32);
  while (acc>>16) {
    acc=(acc&0xffff)+(acc>>16);
  }

  // If the data began at an odd byte address
  // then reverse the byte order to compensate.
  if (offset&1) {
    acc=((acc&0xff00)>>8)|((acc&0x00ff)<<8);
  }

    // this is implememneted. removed conversion. Return the checksum in network byte order.
  return htons(~acc);
}
/*
uint16_t icmp_checksum(uint16_t *buffer, uint32_t size) 
{
    unsigned long cksum=0;
    while(size >1) 
    {
        cksum+=*buffer++;
        size -=sizeof(USHORT);
    }
    if(size ) 
    {
        cksum += *(UCHAR*)buffer;
    }
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >>16);
    return (uint16_t)(~cksum);
}
*/
struct routing_table
{
  struct in_addr s_addr,next_hop;
  char ifname[10];
  struct routing_table *next;
};

struct routing_table *head;

  /* Ethernet header */
struct sniff_ethernet {
  u_char ether_dhost[ETHER_ADDR_LEN]; /* Destination host address */
  u_char ether_shost[ETHER_ADDR_LEN]; /* Source host address */
  u_short ether_type; /* IP? ARP? RARP? etc */
};

  /* IP header */
struct __attribute__((__packed__)) sniff_ip 
{
  u_char ip_vhl;    /* version << 4 | header length >> 2 */
  u_char ip_tos;    /* type of service */
  u_short ip_len;   /* total length */
  u_short ip_id;    /* identification */
  u_short ip_off;   /* fragment offset field */
  #define IP_RF 0x8000    /* reserved fragment flag */
  #define IP_DF 0x4000    /* dont fragment flag */
  #define IP_MF 0x2000    /* more fragments flag */
  #define IP_OFFMASK 0x1fff /* mask for fragmenting bits */
  u_char ip_ttl;    /* time to live */
  u_char ip_p;    /* protocol */
  u_short ip_sum;   /* checksum */
  struct in_addr ip_src;
  struct in_addr ip_dst; /* source and dest address */
};

/*ICMP header */

struct sniff_icmp
{
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint32_t unused;
};



void callback(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char *packet)
{
  static int count = 1;
  struct sniff_ethernet *eth=NULL;
  struct sniff_ip *iph=NULL;
  struct sniff_icmp *icmp=NULL;
  struct routing_table *loop=NULL;
  char rec_dst[30],rt_dst[30],*track_dot=NULL,*temp=NULL,ifname[30],*null=NULL,dev_1[30];
  u_char *s_hw=NULL;
  struct ifreq ifr;
  unsigned char *d_hw=NULL;
  struct arpreq arpreq={0};
  struct sockaddr_in *sin=NULL,*s_ip=NULL;
  struct in_addr next_ip;
  int m=0,flag=0;

  next_ip.s_addr=0;
  rec_dst[0]='\0';
  rt_dst[0]='\0';
  ifname[0]='\0';
  dev_1[0]='\0';
  ifr.ifr_name[0]='\0';
  eth=(struct sniff_ethernet*)packet;
  iph=(struct sniff_ip*)(packet+14);
  
  strcpy(rec_dst,inet_ntoa(iph->ip_dst));

 
  /*Injecting the packet in the network*/

  /*Updating the IP packet headers for ttl and checksum*/
  iph->ip_ttl--;

  char pcap_errbuf[PCAP_ERRBUF_SIZE];
  pcap_errbuf[0]='\0';

  /* Checking if the TTL is 0 and finding the interface to inject */

  if(iph->ip_ttl==0)
     {
	printf(" Inside TTL for \n");
        memcpy(dev_1,useless,4);
        //printf(" Inside TTL for copy successfull :%s , %s\n",dev_1,useless);
      pcap_t* ppcap = pcap_open_live(useless, 98, 0, 0, pcap_errbuf);

      if(ppcap == NULL)
      {
        printf("Not able to open the interface: %s\n", pcap_errbuf);
        return;
      }

      strcpy(ifr.ifr_name,useless);
      printf(" Inside TTL for copy successfull 1 : %s\n",ifr.ifr_name);
      if(ioctl(pcap_fileno(ppcap),SIOCGIFHWADDR,&ifr)<0)
      {
          perror("ioctl 1 :");
      }
      else
      {

        s_hw= (unsigned char*)ifr.ifr_hwaddr.sa_data;
      }
           memcpy(ifname,dev_1,4);
           null=(char*)(packet+14+20+8);
           memmove(null,iph,28);
           iph->ip_ttl=64;
           iph->ip_p=0x01;
           icmp=(struct sniff_icmp*)(packet+14+20);
           icmp->type=11;
           icmp->code=0;
           icmp->unused=(uint32_t)0;
           icmp->checksum = 0;
           icmp->checksum = ip_checksum((char*)(packet+14+20),ntohs(iph->ip_len)-20);
           memcpy(eth->ether_dhost,eth->ether_shost,6);
       	   flag=1;
           iph->ip_dst.s_addr=iph->ip_src.s_addr;
           if(ioctl(pcap_fileno(ppcap),SIOCGIFADDR,&ifr)<0)
      {
          perror("ioctl 2:");
      }
      else
      {

        s_ip= (struct sockaddr_in*)&ifr.ifr_addr;
       }
          iph->ip_src.s_addr=s_ip->sin_addr.s_addr;
      
    }
  else
  {
    /* Checking in the routing table */

      track_dot=rec_dst;
      temp=strchr(track_dot,'.');
      while(temp!=NULL)
      {
       track_dot=temp;
       track_dot++;
       temp=strchr(track_dot,'.');
      }
      *track_dot='\0';

      for(loop=head;loop!=NULL;loop=loop->next)
      {
         strcpy(rt_dst,inet_ntoa(loop->s_addr));
         track_dot=rt_dst;
         while(temp=strchr(track_dot,'.'))
         {
            track_dot=temp;
            track_dot++;
         }
         *track_dot='\0';
         if(strcmp(rt_dst,rec_dst)==0)
         {
           strcpy(ifname,loop->ifname);
           next_ip.s_addr=loop->next_hop.s_addr;
           break;
          }        
      }
}
       /* Injecting the packet into the destination interface */
     
  pcap_t* ppcap = pcap_open_live(ifname, 98, 0, 0, pcap_errbuf);
    if(ppcap == NULL)
  {
    return;
  }

  strncpy(ifr.ifr_name,ifname,15);
  if(ioctl(pcap_fileno(ppcap),SIOCGIFHWADDR,&ifr)<0)
  {
    perror("ioctl 3:");
  }
  else
  {
    s_hw= (unsigned char*)ifr.ifr_hwaddr.sa_data;
   }

  memcpy(eth->ether_shost,s_hw,6);

  /* Finding the destination MAC address */
    if(flag==0)
   {
  memset(&arpreq, 0, sizeof(arpreq));
  sin = (struct sockaddr_in *) &arpreq.arp_pa;
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = next_ip.s_addr;
  strcpy(arpreq.arp_dev,ifname);

 if (ioctl(pcap_fileno(ppcap), SIOCGARP, &arpreq) < 0) 
  {
    perror("ioctl 2");
  }

  if (arpreq.arp_flags & ATF_COM) 
  {
    d_hw = (unsigned char *) &arpreq.arp_ha.sa_data[0];

  } 
 

  memcpy(eth->ether_dhost,d_hw,6);
 }
     
  iph->ip_sum = 0;
  iph->ip_sum = ip_checksum(iph, 20);
 
  int p = pcap_sendpacket(ppcap, packet, ntohs(iph->ip_len)+14);
  if (p != 0)
  {
    pcap_perror(ppcap, "Failed\n");
    pcap_close(ppcap);
  }
  else
  {
    pcap_close(ppcap);
  }

}

/* Sniffing for Packets */

void *sniff_1(void *arg)
{
  u_char *dev_name=(u_char*)arg;
  pcap_t *handle;
  char errbuf[PCAP_ERRBUF_SIZE];

  printf(" Created Thread : %s\n",dev_name);
  handle=pcap_open_live(dev_name,BUFSIZ,0,-1,errbuf);
  if( handle==NULL)
    printf(" Could not create handler : %s \n",errbuf);

  pcap_setdirection(handle, PCAP_D_IN);
  pcap_loop(handle,-1, callback, dev_name); 

}

int main(int argc,char **argv)
{
    char *dev;
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* descr;
    struct bpf_program fp;        /* to hold compiled program */
    bpf_u_int32 pMask;            /* subnet mask */
    bpf_u_int32 pNet;             /* ip address*/
    pcap_if_t *alldevs, *d;
    char dev_buff[64] = {0}, buff[1024];
    int i =0;
    struct in_addr temp_addr;
    pthread_t thread[10];
    struct routing_table *route,*temp;
    char *outinterface="eth2";

    /* Populating ARP tablle */

    sprintf(buff,"sysctl -w net.ipv4.conf.%s.arp_accept=1",outinterface);
    system(buff);
    sprintf(buff,"arpsend -D -e 10.10.1.1 %s",outinterface);
    system(buff);
    system("iptables -I OUTPUT -p icmp --icmp-type destination-unreachable -j DROP");
       
    /* Creating a routing a table */

    route=(struct routing_table*)malloc(sizeof(struct routing_table));
    inet_aton("10.1.0.0", &route->s_addr);
    inet_aton("10.10.1.1", &route->next_hop);
    strncpy(route->ifname,"eth2",4);
    head=route;
    temp=route;

 
    route=(struct routing_table*)malloc(sizeof(struct routing_table));
    inet_aton("10.10.1.0", &route->s_addr);
    inet_aton("10.10.1.1", &route->next_hop);
    strncpy(route->ifname,"eth2",4);
    temp->next=route;
    temp=route; 

    route=(struct routing_table*)malloc(sizeof(struct routing_table));
    inet_aton("10.10.3.0", &route->s_addr);
    inet_aton("10.10.3.2", &route->next_hop);
    strncpy(route->ifname,"eth1",4);
    temp->next=route;
    temp=route; 
 
    temp->next=NULL;

    for (temp=head;temp!=NULL;temp=temp->next)
    {
       printf("IP address : %s \t interface name: %s \n", inet_ntoa(temp->s_addr), temp->ifname);
    }

    // Prepare a list of all the devices
    if (pcap_findalldevs(&alldevs, errbuf) == -1)
    {
        fprintf(stderr,"Error in pcap_findalldevs: %s\n", errbuf);
        exit(1);
    }

    printf("\nHere is a list of available devices on your system:\n\n");
    for(d=alldevs; d; d=d->next)
    {
        if(strncmp (d->name,"eth",3) == 0)
        {
            pcap_lookupnet(d->name, &pNet, &pMask, errbuf);
            temp_addr.s_addr=pNet;
            if(strncmp(inet_ntoa(temp_addr),"10.",3) == 0)
            {
               printf("Inside If \n");
         strcpy(devices[i],d->name);
        printf("Inside If 1\n");
               printf("%d. %s: Network address %s \n", ++i, d->name,inet_ntoa(temp_addr));
               pthread_create(&thread[i],0,sniff_1,d->name);
               
            }
        }
    }
    num_devices=(i-1);
    pthread_join(thread[3],0);
    pthread_join(thread[1],0);
    pthread_join(thread[2],0);
    printf("\nDone with packet sniffing!\n");
    return 0;
}
