ll: Custom

Custom: Custom_IP_ARP.o
         gcc -o Custom -pthread -lm Custom_IP_ARP.o -lpcap

Custom_IP_ARP.o: Custom_IP_ARP.c
         gcc -c  -pthread -lm Custom_IP_ARP.c -lpcap

clean:
         rm Custom_IP_ARP.o custom.exe
