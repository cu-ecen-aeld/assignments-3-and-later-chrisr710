#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include "queue.h"
#define BUFFER_SIZE 2000
#define TIMER_INTERVAL 10
#define REMOTE_IP_SIZE 100
#define TIME_SIZE 70
#include <time.h>
#include <sys/time.h>
#include "aesd_ioctl.h"

bool close_socket_when_receive_delim=false;
bool use_dev=false;
char * outfile="/var/tmp/aesdsocketdata";
bool is_working=true;

//Socket Vars Used Later
struct addrinfo *res; //this is the struct for setting up the socket
int parent_fd; //the fd of the socket before we accept connections
int thread_id_counter=0; //this is so each new thread gets a nice number.
bool should_quit=false; //when this is set to true, the loops stop.
pthread_mutex_t linked_list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;


long handle_ioctl_commainds(char * buffer, long length){
   //printf("checking for length %ld\n",length);
   if ((length < 23) || (length > 27)) {return(-1);} //there is not enough data here
    char orig_char=buffer[length];
    buffer[length] = '\0'; //make a real string out of it, temporarily
    char test_string[]="AESDCHAR_IOCSEEKTO:"; //what we look for
    char *p;
    p = strstr(buffer, test_string);
    if (p == 0){
         buffer[length] = orig_char;    
        return(-1);}
    //this is the command string
    //printf("COMMAND STRING RECEIVED\n");
    char start_buffer_string[4];
    char start_offset_string[4];
    int i=0;
    int j=0;
    int k=0;
    
    for (;i<length;i++){
        
        if (buffer[i] == ':'){break;}
    }
    i++;
    for (;i<length;i++){
        //printf("real buffer at %d = %c\n",i,buffer[i]);
        if (buffer[i] == ','){break;}
        
        else{start_buffer_string[j]=buffer[i];}
            j++;    
        //printf("J=%d\n",j);
    }
        start_buffer_string[j]='\0';
    
    i++;
        
    for (;i<length;i++){
        start_offset_string[k]=buffer[i];
        //printf("real buffer at %d = %c\n",i,buffer[i]);
        if (buffer[i] == '\n'){start_offset_string[i]='\0';}
            
        k++;
        //this will include the null
    }
    
    size_t start_buffer_position=atoi(start_buffer_string);
    size_t start_buffer_offset=atoi(start_offset_string);
    //printf("Start buffer position=%ld ",start_buffer_position);
    //printf("start buffer offset = %ld\n" ,start_buffer_offset);
    
    struct aesd_seekto test = {start_buffer_position,start_buffer_offset,0,buffer};
	//printf("sent request\n");
	int dev = open("/dev/aesdchar",O_WRONLY);
	ioctl(dev, AESDCHAR_IOCSEEKTO, &test);
    //printf("%ld BYTES RETURNED\n", test.length_copied);
	//printf("%s\n",buffer);
	close(dev);
    return(test.length_copied);
} 
    


//Makes a node for the linked list
typedef struct node{
	int id;	
	int fd;
	TAILQ_ENTRY(node) nodes; //linked list stuff
	bool is_done; //when this is done, we can quit.
	int mythread_returnval; //the thread return val that we will use
	pthread_t mythread; //the thread we will use
	char ip_addr[REMOTE_IP_SIZE];
	}node_t;

void get_current_time(char *curr_time){
	//rfc2822: 01 Jun 2016 14:31:46 -0700
	//printf("getting the current time\n");
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	//printf("size of chars:%ld\n",sizeof(curr_time));
	strftime(curr_time,TIME_SIZE,"%a, %d %b %Y %T %z",timeinfo);
	//printf("CURR TIME IN FUNCTION:%s\n",curr_time);
}

void print_time_to_file() {
	char * writestr=malloc(TIME_SIZE + 10);
	char * prefix="timestamp:";
	for (int i=0;i<10;i++)
        {writestr[i]=prefix[i];}
	pthread_mutex_lock(&file_mutex);
	//printf("FILE IS LOCKED\n");
	get_current_time(&writestr[10]);
	//printf("WRITESTR:%s\n",writestr);
	
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int fd=open(outfile,O_WRONLY | O_APPEND | O_CREAT,mode);
	if (fd < 1){
		//printf("ERROR OPENING OUTFILE TO WRITE\n");
		}
	char newline='\n';
	write(fd,writestr,strlen(writestr));
	write(fd,&newline,1); 
	close(fd);
	pthread_mutex_unlock(&file_mutex);
	//printf("FILE IS UNLOCKED\n");
	free(writestr);
	
}
	

void dump_buffer_to_file(long length_to_dump, char * buffer){
	//printf("dumping buffer to file\n");
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	if (! use_dev){
		pthread_mutex_lock(&file_mutex);
	}
	int fd;
	//printf("FILE IS LOCKED\n");
	if (! use_dev){
	fd=open(outfile,O_WRONLY | O_APPEND | O_CREAT,mode);
	}
	else{
	fd=open(outfile,O_WRONLY);	
	}
	if (fd < 1){
		//printf("ERROR OPENING OUTFILE TO WRITE\n");
		if (! use_dev){
			pthread_mutex_unlock(&file_mutex);
		}
		//printf("FILE IS UNLOCKED\n");
		}
	write(fd,buffer,length_to_dump);
	close(fd);
	if (! use_dev){
		pthread_mutex_unlock(&file_mutex);
	}
	//printf("FILE IS UNLOCKED\n");
	}


long find_delimter_position(char * buffer,long buffer_pos,long bytes_received_this_iteration){
	//printf("searching for delimiter starting at position: %ld\n",buffer_pos);
	for (long i= buffer_pos; i<(buffer_pos + bytes_received_this_iteration); i++){
		//printf("looking for delimiter at %ld, found:%c\n",i,buffer[i]);
		if (buffer[i] == '\n'){
			//printf("found delimiter at %ld\n",i);
			return(i);
			}
		}
	return(-1);
	}

int dump_file_to_socket(int socket_fd){
	//printf("the filename I will read to send out data is:%s\n",outfile);
	int file_buffer_size=100;
	char * out_buffer[file_buffer_size];
	
	if (! use_dev){
		pthread_mutex_lock(&file_mutex);
	}
	//printf("FILE IS LOCKED\n");
	int fd=open(outfile, O_RDONLY);
	//printf("FD for outfile=%d\n",fd);
	if (fd < 1){
		//printf("ERROR OPENING OUTFILE TO READ\n");
		if (! use_dev){
			pthread_mutex_unlock(&file_mutex);
		}
		}
	long curr_file_offset=0;
	
	while (1){
			//printf("Reading outfile, curr_bytes_read=%ld\n",curr_file_offset);
			lseek(fd, curr_file_offset, SEEK_SET); //go to curr_offset point in the file...
			int curr_bytes_read = read(fd, out_buffer, file_buffer_size); //read up to the size of the buffer into the buffer
			if (curr_bytes_read > 0){
				send(socket_fd,out_buffer,curr_bytes_read,0); //send up to the amount of bytes read from the buffer.
				curr_file_offset+=curr_bytes_read;
				}
			else{close(fd);
				is_working=false;
				if (! use_dev){
					pthread_mutex_unlock(&file_mutex);
				}
				//printf("FILE IS UNLOCKED\n");
				break;
				}
			}
		return(0);
	}

struct connection_worker_params{
	int fd;
	char ip_addr[50];
	int id;
	};
	
void * connection_worker(void * arg){
	//printf("connection worker started!\n");
	
	char * buffer = malloc(BUFFER_SIZE);
	int buffer_size=BUFFER_SIZE;
	int available_buffer=BUFFER_SIZE;
	int total_bytes_received=0;
	long buffer_position=0;
	long bytes_received;
	long delimiter_position;
	struct node *e=(struct node *) arg;
	int fd=e->fd;
	char remote_ip_str[REMOTE_IP_SIZE];
	//printf("THe configured remote ip address:%s\n",f->ip_addr);
	strcpy(remote_ip_str,e->ip_addr);
	//char *remote_ip_str=f->ip_addr;
	//printf("remote ip:%s\n",remote_ip_str);
	syslog(LOG_INFO,"Accepted connection from %s",remote_ip_str);
	//printf("NEW CONNECTION!\n");
	pthread_mutex_unlock(&linked_list_mutex);
	
	while (1) {	
				if (! use_dev){
					if(should_quit){
						//printf("Listening socket closed because of signal interrupt\n");
						close(fd);
						break;
					}
					//printf("Socket for id %d receiving up to %d bytes\n",f->id,available_buffer);
					bytes_received=recv(fd,buffer+total_bytes_received,available_buffer,0); //receive starting at new buffer
					//printf("node %d bytes_received=%ld\n",f->id,bytes_received);
					if (bytes_received < 1){
											//printf("connection closed; done\n");
											syslog(LOG_INFO,"Closed connection from %s",remote_ip_str);
											//printf("got a hangup\n");
											break;
					}
					total_bytes_received= total_bytes_received+bytes_received;
					//printf("total bytes received:%d\n",total_bytes_received);
					//find_delimter_position(char * buffer,long buffer_pos,long bytes_received_this_iteration)
					delimiter_position=find_delimter_position(buffer,buffer_position,bytes_received);
					if (delimiter_position==-1 && ! use_dev){ //there is more to come
						//printf("The message in the buffer does not contain the delimiter\n");
						//dump_buffer_to_file(bytes_received,buffer);
						buffer_position=buffer_position+bytes_received;
						available_buffer=buffer_size-bytes_received;
						if (buffer_position==buffer_size){  //we have filled the buffer but did not find delimiter yet
							//printf("reallocating buffer\n");
							char*newptr;
							newptr = realloc(buffer,buffer_size+BUFFER_SIZE);
							buffer=newptr;
							buffer_size=buffer_size+BUFFER_SIZE;
							available_buffer=buffer_size-total_bytes_received;
						}
					}
					else{
						//we have found the delimiter
						//printf("dumping buffer to file\n");
						//char instring[500];
						//int x=0;
						//printf("BYTES RECEIVED HERE IS:%ld\n",bytes_received);
						//for (x; x<bytes_received;x++ ){
						//	instring[x]=buffer[x];
							//printf("adding %c to instring\n",buffer[x]);
						//}
						//instring[x+1]='\0';
						//printf("string being recorded: %s\n\n",instring);
						
						dump_buffer_to_file(total_bytes_received,buffer); //now however many bytes were received is dumped to file
						//printf("dumping file to socket\n");
						dump_file_to_socket(fd);
						
						//cleanup and wait for more, or close connection?
						if (!close_socket_when_receive_delim){
							char * pointer = realloc(buffer,BUFFER_SIZE);
							buffer=pointer;
							total_bytes_received=0;
							bytes_received=0;
							buffer_position=0;
							available_buffer=BUFFER_SIZE;
						}
						else{
							break;
						}
						
					}
					//dump_buffer_to_file(bytes_received,buffer); //now however many bytes were received is dumped to file
					//printf("total_bytes_received:%d\n",total_bytes_received);
					
		}
	else{ 
	
		//in this case, everything we receive we dump to the socket. IF it contains the delim, write back the socket
		
			//printf("receiving in dev mode\n");
			bytes_received=recv(fd,buffer,available_buffer,0); //receive starting at new buffer
			if (bytes_received < 1){
										//printf("connection closed; done\n");
										syslog(LOG_INFO,"Closed connection from %s",remote_ip_str);
										//printf("got a hangup\n");
										break;
				}
            //printf("receiginb\n");
            long bytes_received_from_ioctl=handle_ioctl_commainds(buffer, bytes_received);
            //printf("ioctl returned %ld\n",bytes_received_from_ioctl);
            if (bytes_received_from_ioctl >1){
                //printf("Bytes with COMMAND!\n");
                send(fd,buffer,bytes_received_from_ioctl,0); //send up to the amount of bytes read from the socket    
                bytes_received=0;
                continue;
            }
            if (bytes_received_from_ioctl ==0){ //had a command, but didn't return anything
               //send up to the amount of bytes read from the socket 
                //printf("Threw a blank!\n");
                bytes_received=0;
                continue;
            }
			dump_buffer_to_file(bytes_received,buffer);
			//printf("DUMPED %ld bytes to the file",bytes_received);
			delimiter_position=find_delimter_position(buffer,0,bytes_received);
			if (delimiter_position != -1){
				//printf("delimiter received,dumping all previous commands");
				dump_file_to_socket(fd);			
			}
		
		
	}
				
	}	
	//printf("Socket worker exited the while loop.\n");
	close(fd);
	free(buffer);
	return(0);	
}



	
//makes a head for the linked list, using the node struct
typedef TAILQ_HEAD(head_s, node) head_t;
head_t head;

void create_node(int fd, char *remote_ip){
	pthread_mutex_lock(&linked_list_mutex);
	struct node * e = malloc(sizeof(struct node));
	thread_id_counter+=1;
	//printf("creating node %d\n",thread_id_counter);
	e->fd=fd;
	e->id=thread_id_counter;
	e->mythread_returnval=pthread_create(&e->mythread, NULL, &connection_worker,e);
	strcpy(e->ip_addr,remote_ip);
	//e->c = string[c];
    TAILQ_INSERT_TAIL(&head, e, nodes); 
	}

void cleanup(void) {
	//printf("running cleanup\n");
	struct node * e = NULL;
	//printf("joining threads\n");
    TAILQ_FOREACH(e, &head, nodes)
    {
        //printf("thread id %d\n", e->id);
		pthread_join(e->mythread,NULL);
		//printf("closing fd %d\n",e->fd);
		close(e->fd);
    }
	close(parent_fd);
	//printf("deleting linked list\n");
    while (!TAILQ_EMPTY(&head))
    {
        e = TAILQ_FIRST(&head);
        TAILQ_REMOVE(&head, e, nodes);
        free(e);
        e = NULL;
    }
	
	
}



int open_socket(void){
			struct addrinfo hints; //it is going to neeed the hints struct
			syslog(LOG_INFO,"RUNNING MEMSET");
			memset(&hints, 0, sizeof hints); // make sure the struct is empty
			hints.ai_family = AF_INET; //ipv4
			hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
			hints.ai_flags = AI_PASSIVE; // fill in my IP for me
			syslog(LOG_INFO,"RUNNING GETADDR INFO");
			getaddrinfo("0.0.0.0","9000", &hints, &res); //this function sets up the res struct based on hints
			parent_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol); //create the socket file descriptor
			//printf("Parent fd:%d\n",parent_fd);
			const int enable = 1;
			setsockopt(parent_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
			syslog(LOG_INFO,"SET OPTS");
			int binding_output=bind(parent_fd, res->ai_addr, res->ai_addrlen);
			//printf("binding output:%d\n",binding_output);
			syslog(LOG_INFO,"BINDING FINISHED, output is %d",binding_output);
			//printf("Binding returned:%d\n",binding_output);
			if (binding_output != 0){syslog(LOG_INFO,"BINDING UNSUCCESSFUL");
									syslog(LOG_INFO,"EXITING");
									return(1);
									}
			pid_t pid = fork();
			if (pid > 0) { //i am the parent
				return(0);
			}
			if (! use_dev){
				struct itimerval timer;
				// Set the signal handler
				//signal(SIGALRM, print_time_to_file);
				// Set the timer interval 
				timer.it_value.tv_sec = TIMER_INTERVAL;
				timer.it_value.tv_usec = 0;
				timer.it_interval.tv_sec = TIMER_INTERVAL;
				timer.it_interval.tv_usec = 0;
				// Start the timer
				setitimer(ITIMER_REAL, &timer, NULL);
			}
			//printf("listening\n");
			listen(parent_fd, 1);
			
			while(! should_quit){
				struct sockaddr_in remote_addr; //this is where we will put the remote addr info
				socklen_t addr_size; //size of the address info struct above
				addr_size = sizeof remote_addr;
				char remote_ip_str[REMOTE_IP_SIZE];
				int new_socket= accept(parent_fd, (struct sockaddr *)&remote_addr, &addr_size);
				if (should_quit){
					//printf("Not accepting this connection, should quit is set.\n");
					exit(0);
				}
				inet_ntop(AF_INET, &remote_addr.sin_addr, remote_ip_str,addr_size );
				create_node(new_socket,remote_ip_str);
				
				//now start a thread. 
			}
	return(0);
	}

void delete_file(void){
	unlink (outfile); 
	//printf("removed file\n");
	}
	
void sigint_handler(int sig) {
    //printf("Caught signal\n");
    syslog(LOG_INFO,"Caught signal, exiting");
	if (sig == SIGINT){syslog(LOG_INFO,"SIGINT");}
	if (sig == SIGTERM){syslog(LOG_INFO,"SIGTERM");}
	//if (sig == SIGALRM){print_time_to_file(NULL);}
	should_quit=true;
	
	cleanup();
	
	}
			
int main(){
	#ifdef USE_AESD_CHAR_DEVICE
	//printf("USING AESDCHAR DEVICE\n");
	use_dev=true;
	#endif
	if (use_dev){
	outfile="/dev/aesdchar";
	//printf("outfile is %s\n",outfile);
	}
	//printf("BIRD\n");
	signal(SIGINT, sigint_handler);
	signal(SIGTERM, sigint_handler);
	signal(SIGALRM, print_time_to_file);
	if (! use_dev){
		delete_file();
	}
	//printf("Address of first head %p\n",&head);
    TAILQ_INIT(&head); // initialize the head
	//create_node(&head);
	struct node * e = NULL;
    TAILQ_FOREACH(e, &head, nodes)
    {
        //printf("%d\n", e->id);
    }
	
	open_socket();
}
