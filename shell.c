#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>


char cwd[1024];
char buff[1024];
int buffChars = 0;
char *arg_array[64];
int arg_count = 0;
char input;
int inputRedirect = 0;
int outputRedirect = 0;
int outputAppendRedirect = 0;
char *inputFileName;
char *outFileName;
int piping = 0;
int isBackground = 0;

/*****Structure data type to store commands*****/
struct commandList
{
	char *arg_array[64];
	int arg_count;
}command[50];

/*****Signal Handler function*****/
void signal_handler()
{
	printf("\n%s ~ $ ", getcwd(cwd, sizeof(cwd)));
	fflush(stdout);
}

/*****executing all the commands but the last one cinnocted using pipes*****/
void piping_process (int in, int out, struct commandList *cmd)
{
	pid_t pid;
	if ((pid = fork ()) == 0)
	{
		if (in != 0)
  		{
			dup2 (in, 0);
			close (in);
  		}
		if (out != 1)
		{
			dup2 (out, 1);
			close (out);
		}

		if(execvp (cmd->arg_array[0], (char * const *)cmd->arg_array ) < 0)
		{
			printf("Error in executing the command \n");
			exit(1);
		}
	}
}

/*****function to execute commands*****/
void run_command (int n, struct commandList *cmd)
{
	/*commands which do not require forking i.e. builtin commands*****/
	if(strcmp(cmd[0].arg_array[0], "cd")==0)		//chdir command
	{
		if(!cmd[0].arg_array[1])
		{
			if(chdir(getenv("HOME"))!=0);
		}
		else
		{
			if(chdir(cmd[0].arg_array[1])!=0)
			{
				printf("Invalid Path");
			}
		}
	}
	else if(strcmp(cmd[0].arg_array[0], "mkdir")==0)	//mkdir command
	{
		if(mkdir(cmd[0].arg_array[1], 0700)!=0)
		{
			printf("Invalid Name of Directory or Directory already exist \n");
		}
	}
	else if(strcmp(cmd[0].arg_array[0], "rmdir")==0)	//rmdir command
	{
		if(rmdir(cmd[0].arg_array[1])!=0)
		{
			printf("Directory do not exist \n");
		}
	}
	else if(strcmp(cmd[0].arg_array[0], "exit")==0)		//exit command
	{
		exit(0);
	}
	else
	{
	/*****Commands which require forking*****/
		pid_t pid;
  		int i;
  		int ini, out;
		int  status;
  		int in, fd [2];
  		
  		in = 0;
  		
  		for (i = 0; i < n - 1; ++i)
	    	{
	      		pipe (fd);
	      		
	      		piping_process (in, fd [1], cmd + i);
	      		
	      		close (fd [1]);
	      		
	      		in = fd [0];
	    	}
	  		

  		if((pid = fork()) == 0)
  		{
  			if (in != 0)
			{
	    			dup2 (in, 0);
	    			close(in);
			}
			if(outputRedirect == 2)		// '>' operator 
			{
				out = open(outFileName, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
				dup2(out, 1);
				close(out);
      			}
			if(outputAppendRedirect == 2)		// '>>' operator
			{
				out = open(outFileName, O_RDWR | O_APPEND | O_CREAT, 0777);
				dup2(out, 1);
				close(out);
			}
	    		if(inputRedirect == 2)		// '<' operator
	    		{
				ini = open(inputFileName, O_RDONLY);
				dup2(ini, 0);
				close(ini);
	    		}

  			
   			if(execvp (cmd [i].arg_array[0], (char * const *)cmd [i].arg_array )<0)
   			{
   				printf("Error in executing the command \n");
        			exit(1);
   			}
		}
		else
		{
			if(isBackground==0)		// '&' operator
				while (wait(&status) != pid);
		}
	}
}

/*****function to split command into command name and arguments*****/
void parse_command(char *cmd)
{
	char *token;
	token = strtok(cmd, " ");
	while( token != NULL )
   	{
	   	   if(outputRedirect == 0 && outputAppendRedirect == 0 && inputRedirect == 0 && strcmp(token, "|") != 0 && strcmp(token, "<") != 0 && strcmp(token, "&") != 0)
	   	   {
				command[piping].arg_array[arg_count] = token;
				arg_count++;
				command[piping].arg_array[arg_count] = NULL;
	   	   }
	   	   token = strtok(NULL, " ");

	   	   if(outputRedirect==1)
	   	   {
	   	   		outFileName = token;
	   	   		outputRedirect = 2;
	   	   }
	   	   if(outputAppendRedirect==1)
	   	   {
				outFileName = token;
				outputAppendRedirect = 2;
		   }
	   	   if(inputRedirect==1)
	   	   {
	   	   		inputFileName = token;
	   	   		inputRedirect = 2;
	   	   }

   	   if(token)
   	   {
			if(strcmp(token, ">") == 0)
			{
				outputRedirect = 1;
			}
			if(strcmp(token, ">>") == 0)
			{
				outputAppendRedirect = 1;
			}
			if(strcmp(token, "<") == 0 && inputRedirect == 0)
			{
				inputRedirect = 1;
			}
			if(strcmp(token, "&") == 0)
			{
				isBackground = 1;
				command[piping].arg_array[arg_count] = NULL;
				command[piping].arg_count = arg_count;
				arg_count = 0;
				piping++;
			}

			if(strcmp(token, "|") == 0)
			{
				command[piping].arg_array[arg_count] = NULL;
				command[piping].arg_count = arg_count;
				arg_count = 0;
				piping++;
			}
		}
	
   	}
   	run_command(piping+1, command);
}

/*****function to reset the variables after execution of command*****/
void reset()
{
	input = '\0';
	int i=0;
	int j=0;
	for(i=0;i<=piping;i++)
	{
		for(j=0;j<=command[i].arg_count;j++)
		{
			command[i].arg_array[j] = NULL;
		}
	}
	arg_count = 0;
	inputFileName = NULL;
	outFileName = NULL;
	inputRedirect = 0;
	outputRedirect = 0;
	outputAppendRedirect = 0;
	isBackground = 0;
	piping = 0;
	while(buffChars > 0)
	{
		buff[buffChars] = '\0';
		buffChars--;
	}
	buff[0] = '\0';
	buffChars = 0;
}

/*****fuction to get command through stdin*****/
void get_command()
{
	reset();
	while(input!='\n'){

		input = getchar();
		if(input=='\n') break;

		buff[buffChars] = input;
		buffChars++;
	}
	if(buff[0]!='\0')
        parse_command(buff);
	else 
	return;
}

/*****main function*****/
int main(int argc, char **argv, char **envp)
{

	system("clear");

	/* Signal Handler for Ctrl + C */
	signal(SIGINT, SIG_IGN);
	signal(SIGINT, signal_handler);


	while(1){
		printf("%s ~ $ ", getcwd(cwd, sizeof(cwd)));
		get_command();
	}
	return 0;
}
