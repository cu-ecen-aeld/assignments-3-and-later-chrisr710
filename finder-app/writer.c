#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>

int write_string_to_file(const char* writestring, const char* fname){
        int fd;
        fd = open(fname,O_CREAT | O_RDWR | O_TRUNC , S_IRUSR | S_IWUSR);
	if (fd == -1){
		perror("error with making file");
		syslog(LOG_ERR,"error writing to file %d",errno);
		return(-1);
		}
        ssize_t bytes_written;
        bytes_written=write(fd,writestring,strlen(writestring));
	if (bytes_written < 0){
		perror("error with writing to file");
		 syslog(LOG_ERR,"error writing to file %d",errno);
		return(-1);
		}
	
        //printf("bytes written: %ld",bytes_written);
        close(fd);
	return(0);
        }


int main(int argc, char *argv[]) {
	openlog("writer-app", LOG_CONS | LOG_PID, LOG_USER);
	if (argc != 3) {
		printf("first argument is the full path to the file, including filename. Second argument is the string to write.\n");
		return(1);	}
	syslog(LOG_INFO,"Writing %s to %s",argv[2],argv[1]);
	int success;
	success=write_string_to_file(argv[2],argv[1]);
	if (success == 0){
		printf("done\n");
		return(0);
		}
	return(1);
	}




//filepath="$1"
//string_to_write="$2"
//if [ ! -d "$(dirname "$filepath")" ]; then
//        mkdir -p "$(dirname "$filepath")"
//        #echo "made ""(dirname "$filepath")"
//        if [ "$?" != "0" ]; then
//                echo "Could not create path "$filepath""
//                exit 1
//        fi
//fi
//echo "$string_to_write" > "$filepath"
//if [ "$?" != "0" ]; then
//        echo "Could not write string to  "$filepath""
//        exit 1
//fi
