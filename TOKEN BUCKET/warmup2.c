    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
    #include <sys/time.h>
    #include <errno.h>
    #include <unistd.h>
    #include "cs402.h"
    #include <time.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <pthread.h>
    #include "my402list.h"
    
    pthread_mutex_t m1=PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cv =PTHREAD_COND_INITIALIZER;
    int token_count=0;
    int serial_number_token=0;
    int serial_number_packet=0;
    double lambda=500;
    double mu=2857;
    double token_rate=1.5;
    int bucket_size=10;
    int packet_size=3;
    int packet_number=20;
    double reference_time_usec=0;
    double process_time_prev_packet=0;
    double timestamp_packet_enter_q1=0;
    double timestamp_packet_leave_q1=0;
    double timestamp_packet_enter_q2=0;
    double timestamp_packet_leave_q2=0;
    double time_in_q1=0;
    double time_in_q2=0;
    int flag_token_stop=0;
    double inter_arrival_time_sum=0;
    double sum_service_time=0;
    double sum_service_time_s1=0;
    double sum_service_time_s2=0;
    double sum_time_in_q1=0;
    double sum_time_in_q2=0;
    double end_time=0;
    int drop_packet_count=0;

    pthread_t packet_thread;
    pthread_t token_thread;
    pthread_t server_thread_s1;
    pthread_t server_thread_s2; 
    typedef struct packet {
        int serial_number;
        double packet_entry_timestamp;
        double service_time;
        int tokens_required;
        double inter_arrival_time;
    } Input_data;
    My402List list_Q1;
    My402List list_Q2;

    FILE *file_pointer;

    double get_process_time(){
        struct timeval now;
        gettimeofday(&now,0);
        double time_usec=(((double)now.tv_sec)*1000000) + ((double)now.tv_usec);//check for carry
        double net_time_usec=time_usec-reference_time_usec;
        double net_time_ms=net_time_usec/1000;
        return net_time_ms;
    }
// in this function we are pushing the packets into the server threads.
    void moving_packet_Q1_to_Q2(){
        My402ListElem *elem_Q2=My402ListFirst(&list_Q1);
        // picking the first value of the list_Q1
        if(elem_Q2!=NULL){  
            Input_data* cur_val=(Input_data*)(elem_Q2->obj);
            int serial_number_packet_moving=cur_val->serial_number;
            // removing the element the to be pushed.
            My402ListUnlink(&list_Q1,elem_Q2);
            timestamp_packet_leave_q1=get_process_time();
            time_in_q1=timestamp_packet_leave_q1-timestamp_packet_enter_q1;
            sum_time_in_q1=sum_time_in_q1+time_in_q1;
            // decrementing the the token in the token bucket filter.
            token_count=token_count-cur_val->tokens_required;

            printf("%012.3lfms: p%d leaves Q1, time in Q1 = %lfms, token bucket now has %d tokens\n",
            timestamp_packet_leave_q1,
            serial_number_packet_moving,
            time_in_q1,
            token_count);
        // check if the Q2 is empty or not to decide whether the server threads need to be woken up or not.
            if (My402ListEmpty(&list_Q2)==1){
                timestamp_packet_enter_q2=get_process_time();
                printf("%012.3lfms: p%d enters Q2\n",timestamp_packet_enter_q2,serial_number_packet_moving);
                My402ListAppend(&list_Q2,cur_val);
                // here we are invoking the sleeping servers becuase uptil now q2 was empty.
                pthread_cond_broadcast(&cv);
            }
            else{
                timestamp_packet_enter_q2=get_process_time();
                printf("%012.3lfms: p%d enters Q2\n",timestamp_packet_enter_q2,serial_number_packet_moving);
                My402ListAppend(&list_Q2,cur_val);
            }
        }
    }
    // this function sets the attributes of the incomming packets if  FILE MODE = 1.
    void reading_file(){
            char buf[1024];
            fgets(buf,1024,file_pointer);
            buf[strlen(buf)-1]='\0';
            char *start_ptr=buf;
            char *space_ptr=strchr(start_ptr,' ');
            if(space_ptr!=NULL){
                    *space_ptr++='\0';
            }
            char lambda_temp[20];
            strncpy(lambda_temp,start_ptr,strlen(start_ptr));
            lambda_temp[strlen(start_ptr)]='\0';
            //printf("!!!lambda=%s\n",lambda_temp);
            lambda=(atof(lambda_temp))/1000;
            int c=0;
            while(space_ptr!=NULL){
                start_ptr=space_ptr;
                space_ptr=strchr(start_ptr,' ');     
                if(space_ptr!=NULL){
                    *space_ptr++='\0';
                }
                if(strlen(start_ptr)>0){
                    if(c==0){
                        char packet_size_temp[20];
                        strncpy(packet_size_temp,start_ptr,strlen(start_ptr));
                        packet_size_temp[strlen(start_ptr)]='\0';
                        packet_size=atoi(packet_size_temp);
                       // printf("!!!packet size=%s\n",packet_size_temp);
                    }    
                    else{
                        char mu_temp[20];
                        strncpy(mu_temp,start_ptr,strlen(start_ptr));
                        mu_temp[strlen(start_ptr)]='\0';
                        mu=(atof(mu_temp))/1000;
                       // printf("!!!service_time=%s\n",mu_temp);
                    }
                    c++;
                }    
            }
    }
    // The packet arrival thread.

    void* packet_control(void * arg){
        int i=0;
        while(1){
            pthread_mutex_lock(&m1);
            if(file_pointer!=NULL){
                reading_file();    
            }
            pthread_mutex_unlock(&m1);
            // releasing the mutex before sleep.
            usleep((lambda)*1000);
            pthread_mutex_lock(&m1);
            Input_data *elem_Q1=malloc(sizeof(Input_data));
            double timestamp_process=get_process_time();
            double timestamp_packet=timestamp_process-process_time_prev_packet;
            inter_arrival_time_sum=inter_arrival_time_sum+timestamp_packet;
            process_time_prev_packet=timestamp_process;
            serial_number_packet++;
            if(packet_size<=bucket_size){
                printf("%012.3lfms: p%d arrives needs %d tokens, inter-arrival time = %lfms\n",
                    timestamp_process,
                    serial_number_packet,
                    packet_size,timestamp_packet);

                elem_Q1->serial_number=serial_number_packet;
                elem_Q1->packet_entry_timestamp=timestamp_process;
                elem_Q1->service_time=mu;
                elem_Q1->inter_arrival_time=lambda;
                elem_Q1->tokens_required=packet_size;
                timestamp_packet_enter_q1=get_process_time();
                
                printf("%012.3lfms: p%d enters Q1\n",timestamp_packet_enter_q1,serial_number_packet);
                My402ListAppend(&list_Q1,elem_Q1);
                // checking if there are sufficient number of tokens in the bucket filter. 
                if(token_count>elem_Q1->tokens_required){
                    moving_packet_Q1_to_Q2();
                }   
            }
            else{
                printf("%012.3lfms: p%d arrives needs %d tokens, inter-arrival time = %lfms, dropped\n",
                timestamp_process,
                serial_number_packet,
                packet_size,timestamp_packet);
                drop_packet_count++;
            }    
            i++;
            if(serial_number_packet==packet_number){
                pthread_mutex_unlock(&m1);
                return 0;
            }
            pthread_mutex_unlock(&m1);
        }
        return 0;
    }
     // This the token thread.
    void* token_control(void * arg){
        int i=0; 
        while(1){       
            usleep((1/token_rate)*1000000);
            pthread_mutex_lock(&m1);
            token_count++;
            serial_number_token++;
            // getting the process time.
            double timestamp_token=get_process_time();
            
            if(token_count>bucket_size){
                token_count--;
                printf("%012.3lfms: token t%d arrives, dropped\n",timestamp_token,serial_number_token);   
            }
            else{
                printf("%012.3lfms: token t%d arrives, token bucket now has %d tokens\n",timestamp_token,
                        serial_number_token, token_count);
            }
            // pushing the packets from list Q1 to list Q2.
            
            My402ListElem *elem_Q2=My402ListFirst(&list_Q1);
            if(elem_Q2!=NULL){
                Input_data* cur_val=(Input_data*)(elem_Q2->obj);
                if((token_count)==cur_val->tokens_required){
                    moving_packet_Q1_to_Q2();    
                }   
            }
            i++;
            pthread_mutex_unlock(&m1);
        }
        return 0;
    }

    void server_control(int arg){
        int i=1;
        while(1){   
            pthread_mutex_lock(&m1);
            // wait in the CV queue until  a packet commes in q2.
            while(My402ListEmpty(&list_Q2)==1){
                pthread_cond_wait(&cv, &m1);
            }
            My402ListElem *elem_Q2=My402ListFirst(&list_Q2);
            if(elem_Q2!=NULL){
                Input_data* cur_val=(Input_data*)(elem_Q2->obj);
                int serial_number_packet_moving=cur_val->serial_number;
                double packet_start_process=cur_val->packet_entry_timestamp;
                My402ListUnlink(&list_Q2,elem_Q2);
                timestamp_packet_leave_q2=get_process_time();
                time_in_q2=timestamp_packet_leave_q2-timestamp_packet_enter_q2;
                sum_time_in_q2=sum_time_in_q2+time_in_q2;
                printf("%012.3lfms: p%d leaves Q2, time in Q2 = %lfms\n",
                    timestamp_packet_leave_q2,
                    serial_number_packet_moving,
                    time_in_q2);
                double packet_arrival=get_process_time();
                printf("%012.3lfms: p%d begins service at S%d, requesting %lfms of service\n",
                    packet_arrival,
                    serial_number_packet_moving,
                    arg,
                    (cur_val->service_time)*1000);
                pthread_mutex_unlock(&m1);
                // emulating time needed to process the packet.
                usleep((cur_val->service_time)*1000);  
                pthread_mutex_lock(&m1);
                double packet_depart_time=get_process_time();
                double service_time=packet_depart_time-packet_arrival;
                double time_in_system=packet_depart_time-packet_start_process;
                sum_service_time=sum_service_time+service_time;
                printf("%012.3lfms: p%d departs from S%d, service time = %lfms, time in system = %lfms\n",
                    packet_depart_time, 
                    serial_number_packet_moving,
                    arg, 
                    service_time,
                    time_in_system);
                if(arg==1){
                    sum_service_time_s1=sum_service_time_s1+ service_time;   
                }
                else if(arg==2){
                    sum_service_time_s2=sum_service_time_s2+ service_time;
                }
                i++;
                // checking if all the packets have been received then kiiing other threads.
               if(serial_number_packet_moving==packet_number){
                    pthread_cancel(token_thread);
                    if(arg==1){
                        pthread_cancel(server_thread_s2);
                    }
                    else if(arg==2){
                        pthread_cancel(server_thread_s1);    
                    }
                    end_time=get_process_time();
                    printf("%012.3lfms: emulation ends\n",end_time);   
                    pthread_mutex_unlock(&m1);
                    break;
            }
            pthread_mutex_unlock(&m1); 
            }       
        }
    }
    // invking the server thread-1
    void* server_control_s1(void * arg){
        int s1=1;
        server_control(s1); 
        return 0; 
    }
    // invoking the server thread-2
    void* server_control_s2(void * arg){
        int s2=2;
        server_control(s2);
        return 0; 
    }
    // utlity functions.
    void calc_lambda(int i,char *argv[]){
        if(argv[i+1]!=NULL){
            if(strchr(argv[i+1],'-')!=NULL){
                printf("Only positive values of lambda are allowed\n");
                exit(1);
            }
            double dval=(double)0;
            if(sscanf(argv[i+1],"%lf",&dval)!=1){
                printf("cannot parse argv[%d] to get a double value\n",i+1);
            }
            else{
                if(dval>2147483647){
                    printf("Lambda can be a positive integer with a maximum value of 2147483647\n");
                    exit(1);   
                }
                lambda=dval;
            }
        }
    }

    void calc_mu(int i,char *argv[]){
        if(argv[i+1]!=NULL){
            if(strchr(argv[i+1],'-')!=NULL){
                printf("Only positive values of mu are allowed\n");
                exit(1);
            }
            double dval=(double)0;
            if(sscanf(argv[i+1],"%lf",&dval)!=1){
                printf("cannot parse argv[%d] to get a double value\n",i+1);
            }
            else{
                if(dval>2147483647){
                    printf("mu can be a positive integer with a maximum value of 2147483647\n");
                    exit(1);   
                }
                mu=dval;
            }
        }
    }

    void calc_rate(int i,char *argv[]){
        if(argv[i+1]!=NULL){
            if(strchr(argv[i+1],'-')!=NULL){
                printf("Only positive values of r are allowed\n");
                exit(1);
            }
            double dval=(double)0;
            if(sscanf(argv[i+1],"%lf",&dval)!=1){
                printf("cannot parse argv[%d] to get a double value\n",i+1);
            }
            else{
                if(dval>2147483647){
                    printf("r can be a positive integer with a maximum value of 2147483647\n");
                    exit(1);   
                }
                token_rate=dval;
            }
        }
    }

    void calc_bucket_size(int i,char *argv[]){
        if(argv[i+1]!=NULL){
            int dval=(int)0;
            if(sscanf(argv[i+1],"%d",&dval)!=1){
                printf("cannot parse argv[%d] to get a double value\n",i+1);
            }
            else{
                bucket_size=dval;
            }
        }
    }

    void calc_packet_size(int i,char *argv[]){
        if(argv[i+1]!=NULL){
            int dval=(int)0;
            if(sscanf(argv[i+1],"%d",&dval)!=1){
                printf("cannot parse argv[%d] to get a double value\n",i+1);
            }
            else{
                packet_size=dval;
            }
        }
    }

    void calc_packet_number(int i,char *argv[]){
        if(argv[i+1]!=NULL){
            int dval=(int)0;
            if(sscanf(argv[i+1],"%d",&dval)!=1){
                printf("cannot parse argv[%d] to get a double value\n",i+1);
            }
            else{
                packet_number=dval;
            }
        }
    }

    void get_file_input(int i,char *argv[]){
        if(argv[i+1]!=NULL){
            file_pointer=fopen(argv[2],"r");
            if(file_pointer==NULL){
                perror("Error: ");
                fprintf(stderr,"usage: warmup2 [-t tfile]\n");
                exit(1);
            }

        char buf[1024];
        pthread_mutex_lock(&m1);
        fgets(buf,1024,file_pointer);
        buf[strlen(buf)-1]='\0';
        packet_number=atoi(buf);  
        pthread_mutex_unlock(&m1); 
        }
    }

    void validating_command_line(int argc, char *argv[])
    {
        int flag_input_file=0;
        int i=0;
        int file_name_index=0;
        for(i=1;i<argc;i++){
            if(strcmp(argv[i],"-n")==0){
               calc_packet_number(i,argv); 
            }
            if(strcmp(argv[i],"-lambda")==0){
               calc_lambda(i,argv); 
            }
            if(strcmp(argv[i],"-mu")==0){
               calc_mu(i,argv); 
            }
            if(strcmp(argv[i],"-r")==0){
               calc_rate(i,argv); 
            }
            if(strcmp(argv[i],"-B")==0){
               calc_bucket_size(i,argv); 
            }
            if(strcmp(argv[i],"-P")==0){
               calc_packet_size(i,argv); 
            }
            if(strcmp(argv[i],"-t")==0){
               get_file_input(i,argv); 
               flag_input_file=1;
               file_name_index=i+1;
            }
        }
        printf("    number to arrive = %d\n",packet_number);
        if(flag_input_file==0){
            printf("    lambda = %lf\n",lambda);
            printf("    mu = %lf\n",mu);
            printf("    r = %lf\n",token_rate);
            printf("    B = %d\n",bucket_size);
            printf("    P = %d\n",packet_size);
        }
        else{
            printf("    r = %lf\n",token_rate);
            printf("    B = %d\n",bucket_size);
            printf("    tsfile = %s\n",argv[file_name_index]);    
        }
        printf("\n");
    }
     // we are displaying statistics
    void display_statistics(){
        printf("\nStatistics:\n");

        double inter_arrival_time_avg=inter_arrival_time_sum/packet_number;
        double avg_service_time=sum_service_time/packet_number;
        double avg_time_in_q1=sum_time_in_q1/end_time;
        double avg_time_in_q2=sum_time_in_q2/end_time;
        double avg_time_in_s1=sum_service_time_s1/end_time;
        double avg_time_in_s2=sum_service_time_s2/end_time;


        printf("    average packet inter-arrival time = %.6g\n",inter_arrival_time_avg);
        printf("    average packet service time = %.6g\n",avg_service_time);
        printf("    average number of packets in Q1 = %.6g\n",avg_time_in_q1);
        printf("    average number of packets in Q2 = %.6g\n",avg_time_in_q2);
        printf("    average number of packets in S1 = %.6g\n",avg_time_in_s1);
        printf("    average number of packets in S2 = %.6g\n",avg_time_in_s2);
    }

    int main(int argc, char *argv[]){
        
        printf("\nEmulation Parameters:\n");
        validating_command_line(argc,argv);
        struct timeval now;
        gettimeofday(&now,0);
        reference_time_usec=((double)now.tv_sec*1000000) + (double)now.tv_usec;
        printf("00000000.000:ms emulation begins\n");
        memset(&list_Q1, 0, sizeof(My402List));
        (void)My402ListInit(&list_Q1);
        memset(&list_Q2, 0, sizeof(My402List));
        (void)My402ListInit(&list_Q2);
        void *result=NULL;
        pthread_create(&packet_thread, 0, packet_control, 0);
        pthread_create(&token_thread, 0, token_control, 0);
        pthread_create(&server_thread_s1, 0, server_control_s1, 0);
        pthread_create(&server_thread_s2, 0, server_control_s2, 0);
        pthread_join(packet_thread, (void**)&result); 
        pthread_join(token_thread, (void**)&result);
        pthread_join(server_thread_s1, (void**)&result);
        pthread_join(server_thread_s2, (void**)&result);

        display_statistics();
        
    return 0;
    }

