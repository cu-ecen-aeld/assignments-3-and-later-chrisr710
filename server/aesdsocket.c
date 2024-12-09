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
//make file opener, writeable, cleared on open
char * outfile="/var/tmp/aesdsocketdata";
bool is_working=false; 
int parent_fd=0;
int child_fd=0;
char * curr_buff = NULL;
int open_item=0;
struct addrinfo *res; //this is the struct for setting up the socket
bool am_parent=true;
bool should_quit=false;
bool done_quitting=false;

long find_delimter_position(char * buffer,long buffer_pos,long bytes_received_this_iteration){
	printf("searching for delimiter starting at position: %ld\n",buffer_pos);
	for (long i= buffer_pos; i<(buffer_pos + bytes_received_this_iteration); i++){
		//printf("looking for delimiter, found:%c\n",buffer[i]);
		if (buffer[i] == '\n'){
			printf("found delimiter at %ld\n",i);
			return(i);
			}
		}
	return(-1);
	}

int dump_file_to_socket(int socket_fd,long file_position){
	is_working=true;
	open_item=1;
	printf("the filename I will read to send out data is:%s\n",outfile);
	printf("sending up to %ld\n",file_position);
	int file_buffer_size=100;
	char * out_buffer[file_buffer_size];
	int out_buffer_position;
	int fd=open(outfile, O_RDONLY);
	printf("FD for outfile=%d\n",fd);
	if (fd < 1){
		printf("ERROR OPENING OUTFILE TO READ\n");
		}
	long curr_file_offset=0;
	int bytesRead=0;
	while (1){
			printf("Reading outfile, curr_bytes_read=%ld\n",curr_file_offset);
			lseek(fd, curr_file_offset, SEEK_SET); //go to curr_offset point in the file...
			int curr_bytes_read = read(fd, out_buffer, file_buffer_size); //read up to the size of the buffer into the buffer
			if (curr_bytes_read > 0){
				send(socket_fd,out_buffer,curr_bytes_read,0); //send up to the amount of bytes read from the buffer.
				curr_file_offset+=curr_bytes_read;
				}
			else{close(fd);
				is_working=false;
				break;
				}
			}
		return(0);
	}

int dump_buffer_to_file(long length_to_dump, char * buffer){
	printf("opening %s for writing...\n",outfile);
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int fd=open(outfile,O_WRONLY | O_APPEND | O_CREAT,mode);
	if (fd < 1){
		printf("ERROR OPENING OUTFILE TO WRITE\n");
		}
	write(fd,buffer,length_to_dump);
	close(fd);
	}
	
int connection_worker(int fd,long myproc, char * remote_ip_str){
	if (myproc == 0){
					close(parent_fd);
					child_fd=fd;
					am_parent=false;
					printf("I am the child\n");
					//close(s);//child doesn't need the original fd.
					long buffer_init_size=10;
					long buffer_size=buffer_init_size;
					char * buffer = malloc(buffer_init_size);
					curr_buff=buffer;
					long buffer_position=0;
					long bytes_received;
					long end_address_of_completed_message=0;
					long delimiter_position;
					
					while (1) {
						//if (should_quit){printf("should quit, breaking\n");break;}
						printf("receiving..., \n");
						is_working=true;
						open_item=2;
						bytes_received=recv(fd,buffer,sizeof(buffer),0);
						if (bytes_received <1){
								is_working=false;
								printf("connection closed; done\n");
								syslog(LOG_INFO,"Closed connection from %s",remote_ip_str);
								printf("got a hangup");
								//should we print out the buffer here?
								printf("exiting from socket worker\n");
								exit(0);
								}
						//printf("DUMPING BUFF\n");
						dump_buffer_to_file(bytes_received,buffer); //now however many bytes were received is dumped to file
						printf("received bytes:%ld\n",bytes_received);
						delimiter_position=find_delimter_position(buffer,0,bytes_received);
						if (delimiter_position==-1){ //there is more to come
							printf("The message in the buffer does not contain the delimiter\n");
							bytes_received=0;
							}
						else{
							//we have found the delimiter
							printf("dumping file to socket\n");
							dump_file_to_socket(fd,buffer_init_size);
							}
						}
					printf("Socket worker exited the while loop.\n");
					close(fd);
					free(buffer);
					}
			return(0);
	}
			
				
	

int open_socket(void){
			int s; //fd for socket
	
			//here we set up a socket. CHILD should never run this.
			
			int s_accepted; //fd for socket accepted
			struct addrinfo hints; //it is going to neeed the hints struct
			syslog(LOG_INFO,"RUNNING MEMSET");
			memset(&hints, 0, sizeof hints); // make sure the struct is empty
			hints.ai_family = AF_INET; //ipv4
			hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
			hints.ai_flags = AI_PASSIVE; // fill in my IP for me
			syslog(LOG_INFO,"RUNNING GETADDR INFO");
			getaddrinfo("127.0.0.1","9000", &hints, &res); //this function sets up the res struct based on hints
			//res now contains an sock_addrin pointer at ai_addr
			
			struct sockaddr_in remote_addr; //this is where we will put the remote addr info
			socklen_t addr_size; //size of the address info struct above
			addr_size = sizeof remote_addr;
			
			s = socket(res->ai_family, res->ai_socktype, res->ai_protocol); //create the socket file descriptor
			parent_fd=s;
			const int enable = 1;
			setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
			syslog(LOG_INFO,"SET OPTS");
			printf("binding\n");
			
			int binding_output=bind(s, res->ai_addr, res->ai_addrlen);
			syslog(LOG_INFO,"BINDING FINISHED, output is %d",binding_output);
			printf("Binding returned:%d\n",binding_output);
			if (binding_output != 0){syslog(LOG_INFO,"BINDING UNSUCCESSFUL");
									syslog(LOG_INFO,"EXITING");
									exit(-1);
									}
			
			while (1){
				printf("listening\n");
				int listening_output=listen(s, 1);
				syslog(LOG_INFO,"LISTENING FINISHED, returned %d",listening_output);
				printf("Listening returned:%d\n",listening_output);
				if (listening_output != 0){ 
											printf("ERROR SETTING UP NEW CONNECTION\n");
										printf("May be shutting down...\n");
										exit(0);
										}
				int pid=fork();
				if (pid!=0){
							break;
							}	
					
				int new_connection = accept(s, (struct sockaddr *)&remote_addr, &addr_size); //will block here until it gets a new connection
				if (new_connection == 0){
										printf("ERROR SETTING UP NEW CONNECTION\n");
										printf("May be shutting down...\n");
										exit(0);
										}
				char remote_ip_str[100];
				inet_ntop(AF_INET, &remote_addr.sin_addr, remote_ip_str,addr_size );
				printf("Received connection from: %s\n",remote_ip_str);
				syslog(LOG_INFO,"Accepted connection from %s",remote_ip_str);
				long new_proc=fork();
				connection_worker(new_connection,new_proc,remote_ip_str);
				
			}
	close(s);
	freeaddrinfo(res); // free the linked-list	
	if (curr_buff != NULL){
						free(curr_buff);
	printf("EXITING FROM PROC\n");				
						}
	
    exit(0); // Exit gracefully
	}

int delete_file(void){
	int unlink_output=unlink (outfile); 
	printf("removed file\n");
	}

void sigint_handler(int sig) {
	//should_quit=true;
    printf("Caught signal\n");
    syslog(LOG_INFO,"Caught signal");
	if (sig == SIGINT){printf("SIGINT\n");}
	if (sig == SIGTERM){printf("SIGTERM\n");}
	should_quit=true;
	close(parent_fd);
	close(child_fd);
	}




int main(int argc, char * argv[]) {
printf("MY SOCKET PROGRAM IS STARTING\n");
syslog(LOG_INFO,"PROGRAM IS STARTING");
openlog("aesdsocket", LOG_CONS | LOG_PID, LOG_USER);
//signal(SIGINT, sigint_handler);
//signal(SIGTERM, sigint_handler);
printf("starting...\n");
delete_file();
syslog(LOG_INFO,"OPENING SOCKET");
open_socket();
exit(0);
}

