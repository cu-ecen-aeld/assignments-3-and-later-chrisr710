#include "systemcalls.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include <fcntl.h>
#include <sys/wait.h>
char * myname="parent";
/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    //printf("doing system: %s\n",cmd);
    int var=0;
    var=system(cmd);
    //printf("var=%d\n",var);
    //printf("returning\n");
    if (var == 0){    
		return true;
		}
    return(false);
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    //command[count] = command[count];
    //command[count] = command[count];
    char * command_to_run=command[0];
    int fork_ret;
    fork_ret=fork();
    printf("fork return:%d\n",fork_ret);
    if (fork_ret <0){printf("fork return is less than 1\n");return(false);}
    if (fork_ret >0){
                        //this is the parent
                        int status = 0;
                        printf("The new process is: %d\n",fork_ret);
                        wait(&status);
                        printf("exit status of the new process: %d\n",WEXITSTATUS(status));
                        if (status==0){
                                return(true);
                                        }
                        else{
                                return(false);
                                }
                    }
     if (fork_ret==0){ //this is the child
                printf("setting myname to child\n");
                myname="child";
                //int fd;
                int ret=0;
                //dup2(fd,1);
                ret = execv(command_to_run,command);
                printf("ret=%d\n",ret);
                printf("returning false\n");
                exit(1);
                        }

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/






    va_end(args);

    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    printf("output file is:%s\n",outputfile);
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
        printf("command at [%d] is %s\n",i,command[i]);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];
    char * command_to_run=command[0];
    int fork_ret;
    fork_ret=fork();
    printf("fork return:%d\n",fork_ret);
    if (fork_ret <0){printf("fork return is less than 1\n");return(false);}
    if (fork_ret >0){
			//this is the parent
			int status = 0;
                        printf("The new process is: %d\n",fork_ret);
			wait(&status);
			printf("exit status of the new process: %d\n",WEXITSTATUS(status));
			if (status==0){
				return(true);
					}
			else{
				return(false);
				}
		    }
    if (fork_ret==0){ //this is the child
		printf("setting myname to child\n");
		myname="child";
        	int fd;
    		fd=open(outputfile,O_TRUNC | O_WRONLY | O_CREAT, 0644);
    		if (fd < 0) { perror("open"); exit(1); }
    		int ret=0;
    		dup2(fd,1);
    		ret = execv(command_to_run,command);
    		printf("ret=%d\n",ret);
    		printf("returning false\n");
    		exit(1);
			}
    



/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    va_end(args);
    return true;
}
//int main( int argc, char *argv[])
//{
//printf("arg is %s\n",argv[1]);
//int cmdlen=argc -1;
//printf("cmdlen=%d\n",cmdlen);
//bool x;
//x=do_system(argv[1]);
//do_exec_redirect("null",cmdlen,argv);
//x=do_exec_redirect("dummy_ouptut_file",2,"/usr/bin/echo","/home/school/school-repository/examples/systemcalls");
//x=do_exec(2,"/usr/bins/echo","hi there!");
//x=do_system("/usr/bind/echo hello");
//printf("Final Output %s: %s\n", myname,x ? "true" : "false");
//}

