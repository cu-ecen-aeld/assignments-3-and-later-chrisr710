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

//Make it run as a daemon, forked
//make file opener, writeable, cleared on open

long find_delimter_position(char * buffer,long buffer_pos,long bytes_received_this_iteration){
	printf("searching for delimiter starting at position: %ld\n",buffer_pos);
	for (long i= buffer_pos; i<(buffer_pos + bytes_received_this_iteration); i++){
		printf("looking for delimiter, found:%c\n",buffer[i]);
		if (buffer[i] == '\n'){
			printf("found delimiter at %ld\n",i);
			return(i);
			}
		}
	return(0);
	}

int open_socket(void){
	//You’ll load this struct up a bit(struct addrinfo.), and then call getaddrinfo(). It’ll return a pointer to a new linked list of these structures 
	//filled out with all the goodies you need.
	//ai_addr field in the struct addrinfo is a pointer to a struct sockaddr. This is where we start getting into the nitty-gritty details of what’s inside an IP address structure.
	//Anyway, the struct sockaddr holds socket address information for many types of sockets.
	//To deal with struct sockaddr, programmers created a parallel structure: struct sockaddr_in (“in” for “Internet”) to be used with IPv4.
	//And this is the important bit: a pointer to a struct sockaddr_in can be cast to a pointer to a struct sockaddr and vice-versa. So even though connect() wants a struct sockaddr*,
	//you can still use a struct sockaddr_in and cast it at the last minute
	int s; //fd for socket
	int s_accepted; //fd for socket accepted
	struct addrinfo hints; //it is going to neeed the hints struct
	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_INET; //ipv4
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me
	struct addrinfo *res; //this is the struct for setting up the socket
	getaddrinfo("10.0.2.15","9000", &hints, &res); //this function sets up the res struct based on hints
	//res now contains an sock_addrin pointer at ai_addr
	
	struct sockaddr_in remote_addr; //this is where we will put the remote addr info
	socklen_t addr_size; //size of the address info struct above
	addr_size = sizeof remote_addr;
	
	s = socket(res->ai_family, res->ai_socktype, res->ai_protocol); //create the socket file descriptor
	const int enable = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	printf("binding\n");
	int binding_output=123;
	binding_output=bind(s, res->ai_addr, res->ai_addrlen);
	printf("Binding returned:%d\n",binding_output);
	printf("listening\n");
	int is_listening=200;
	is_listening=listen(s, 1000);
	printf("Listening returned:%d\n",is_listening);
	char remote_ip_str[100];
	int new_connection;
	new_connection = accept(s, &remote_addr, &addr_size);
	inet_ntop(AF_INET, &remote_addr.sin_addr, remote_ip_str,addr_size );
	printf("Received connection from: %s\n",remote_ip_str);
	int myproc;
	myproc=fork();
	if (myproc == 0){
		printf("I am the child\n");
		close(s);//child doesn't need the original fd.
		long buffer_init_size=10;
		long buffer_size=buffer_init_size;
		char * buffer = malloc(buffer_init_size);
		long buffer_position=0;
		long available_bytes_to_write=buffer_init_size - buffer_position;
		long bytes_received;
		long end_address_of_completed_message=0;
		
		while (1) {
			
			printf("receiving..., available_bytes_to_write=%ld\n",available_bytes_to_write);
			bytes_received=recv(new_connection,buffer + buffer_position,available_bytes_to_write,0);
			if (bytes_received <1){
					printf("connection closed; done\n");
					//should we print out the buffer here?
					return(1);
					}
			printf("received bytes:%ld\n",bytes_received);
			//check for delimiter
			
			end_address_of_completed_message=find_delimter_position(buffer,buffer_position,bytes_received);
			buffer_position+=bytes_received;
			if (end_address_of_completed_message == 0){ //there is more to come
			printf("The message in the buffer does not contain the delimiter\n");
			
				if (buffer_position == buffer_size){
					// we have filled up the buffer, but there is no delimiter. Expand buffer and receive more.
						printf("filled up buffer, no delimiter. Expanding buffer size\n");
						buffer_size+=buffer_init_size;
						buffer = realloc(buffer, buffer_size);
						available_bytes_to_write=buffer_init_size;
						}
				else	{
						printf("Received data, no delimiter..\n");
						
						available_bytes_to_write-=bytes_received;
						}
				printf("buffer position: %ld\n",buffer_position);
				printf("available bytes to write: %ld\n",available_bytes_to_write);
				}
						
			//received completed mssg,	
			else {
				printf("end adress of completed message:%ld\n",end_address_of_completed_message);
				printf("complete message received\n");
				//Now sendit
				send(new_connection,buffer,buffer_position,0);
				for (long i=0;i<buffer_position;i++){
					//printf("%c",buffer[i]);
					
					//printf("\n");
				}
				buffer_size=buffer_init_size;
				buffer_position=0;
				available_bytes_to_write=buffer_size;
				buffer = realloc(buffer, buffer_size);
				
				//return(0);
				//break;
				}
			
		}
	}
		
	
	else{
	printf("I am the daddy, my child is %d\n",myproc);
	}
	freeaddrinfo(res); // free the linked-list
	
	return(0);
}

int initialize_file(const char * fname){
	
	
	
	
}




int main(int argc, char * argv[]) {
printf("hello\n");
open_socket();



}

