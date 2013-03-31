/***********************************************************************************************

 CSci 4061 Spring 2013
 Assignment# 3: Piper program for executing pipe commands 

 Student name: <full name of first student>   
 Student ID:   <first student's ID>   

 Student name: <full name of second student>   
 Student ID:   <Second student's ID>   

 X500 id: <id1>, <id2 (optional)>

 Operating system on which you tested your code: Linux, Unix, Solaris, MacOS
 CSELABS machine: <machine you tested on eg: xyz.cselabs.umn.edu>

 GROUP INSTRUCTION:  Please make only ONLY one  submission when working in a group.
***********************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define DEBUG

#define MAX_INPUT_LINE_LENGTH 2048 // Maximum length of the input pipeline command
                                   // such as "ls -l | sort -d +4 | cat "
#define MAX_CMDS_NUM   8           // maximum number of commands in a pipe list
                                   // In the above pipeline we have 3 commands
#define MAX_CMD_LENGTH 256         // A command has no more than 255 characters

#define READY 0					// Used for logstate
#define TRUE 1
#define FALSE 0

#define DONE 255
FILE *logfp;

char pipe_done = FALSE;
char pipe_bad = FALSE;
int num_cmds = 0;
char *cmds[MAX_CMDS_NUM];
int cmd_pids[MAX_CMDS_NUM];
int cmd_status[MAX_CMDS_NUM]; 


/*******************************************************************************/
/*   The function parse_command_line will take a string such as
     ls -l | sort -d +4 | cat | wc
     given in the char array commandLine, and it will separate out each pipe
     command and store pointer to the command strings in array "cmds"
     For example:
     cmds[0]  will pooint to string "ls -l"
     cmds[1] will point to string "sort -d +4"
     cmds[2] will point to string "cat"
     cmds[3] will point to string "wc"

     This function will write to the LOGFILE above information.
*/
/*******************************************************************************/

int parse_command_line (char commandLine[MAX_INPUT_LINE_LENGTH], char* cmds[MAX_CMDS_NUM]){
    int i = 0;
    char ** save;
    char * command;

    command = strtok(commandLine, "|");							// Tokenize first part of commandLine
    while(command != NULL)										// Keep tokenizing while we still have commands
    {
        if(i == MAX_CMDS_NUM)                               	// Check if there are too many commands
        {
            printf("Too many commands!\n");
            fprintf(logfp, "Too many commands in this command line:\n%s\n", commandLine);
            return -1;
        }
        cmds[i] = command;                      				// Save the command
        fprintf(logfp, "Command %d info: %s\n", i, command);	// Record to LOGFILE
        i++;
        command = strtok(NULL, "|");
    }
    fprintf(logfp, "Number of commands from the input : %d\n", i);	// Record number of commands to LOGFILE
    return i;														// Return the number of commands
}

/*******************************************************************************/
/*  parse_command takes command such as  
    sort -d +4
    It parses a string such as above and puts command program name "sort" in
    argument array "cmd" and puts pointers to ll argument string to argvector
    It will return  argvector as follows
    command will be "sort"
    argvector[0] will be "sort"
    argvector[1] will be "-d"
    argvector[2] will be "+4"
/
/*******************************************************************************/

void parse_command(char input[MAX_CMD_LENGTH],
                   char command[MAX_CMD_LENGTH],
                   char *argvector[MAX_CMD_LENGTH])
{
    int i = 0;
    char * token;
    char ** save;
    
    token = command = strtok(input, " ");	// tokenize first part of the command, save it to command
    while(token != NULL)					// while we have another token
    {
        argvector[i++] = token;				// Save token to argvector
        token = strtok(NULL, " ");			
    }
    argvector[i] = 0;                       // Used to determine the end of arg vector
}


/*******************************************************************************/
/*  The function print_info will print to the LOGFILE information about all    */
/*  processes  currently executing in the pipeline                             */
/*  This printing should be enabled/disabled with a DEBUG flag                 */
/*******************************************************************************/

void print_info(char* cmds[MAX_CMDS_NUM],
                int cmd_pids[MAX_CMDS_NUM],
                int cmd_stat[MAX_CMDS_NUM],
                int num_cmds)
{
    #ifdef DEBUG
    int i;
    if(pipe_done)
		fprintf(logfp, "PID        COMMAND        EXIT\n");
	else
		fprintf(logfp, "PID        COMMAND\n");
    for(i = 0; i < num_cmds; i++)
    {
		if(pipe_done)
		{
			fprintf(logfp, "%-6d\t%s\t%6d\n", cmd_pids[i], cmds[i], cmd_stat[i]);
		}
		else
		{
			fprintf(logfp, "%-6d\t%s\n", cmd_pids[i], cmds[i]);
		}
    }
    #endif
}  



/*******************************************************************************/
/*     The create_command_process  function will create a child process        */
/*     for the i'th command                                                    */
/*     The list of all pipe commands in the array "cmds"                       */
/*     the argument cmd_pids contains PID of all preceding command             */
/*     processes in the pipleine.  This function will add at the               */
/*     i'th index the PID of the new child process.                            */
/*******************************************************************************/


void create_command_process (char cmds[MAX_CMD_LENGTH],  // Command line to be processed
                     int cmd_pids[MAX_CMDS_NUM],          // PIDs of preceding pipeline processes
                                                          // Insert PID of new command processs
		             int i)                               // commmand line number being processed
{
    char * argvector[MAX_CMD_LENGTH];
    char command[MAX_CMD_LENGTH];
    int pipeid[2];
    static int oldpiperead;
    
    parse_command(cmds, command, argvector);
    
    if(i != (num_cmds - 1))                                 // If not the last command
    {
        pipe( pipeid );                                     // Create a new pipeline 
    }
    
    if((cmd_pids[i] = fork()))                 	// Fork Here
    {
		if(i != (num_cmds - 1))								// if not the last command
		{
			oldpiperead = pipeid[0];						// save the read end of the pipe
			close( pipeid[1] );
		}
		else if ( i != 0 )									// if not the first and last command
		{
			close( pipeid[1] );								// close the pipes if 
			close( pipeid[0] );
		}
        return;                                 // Parent Exits Here
    }
    
    else
    {											// Child continues here
		if(i != 0)                                      	// link the previous pipe's read
		{
			dup2(oldpiperead, 0);
			close( oldpiperead );
		}
		
		if(i != (num_cmds - 1))                        		// if not the last command
		{
			dup2(pipeid[1], 1);								// dup stdout to pipe's write
		}
		close(pipeid[0]);									// close pipeids
		close(pipeid[1]);

		if(execvp(argvector[0], argvector))            		// Exec command if child
		{
			perror("execvp: ");								// If exec fails do this
			fprintf(logfp, "An Execution error occured with process %d terminating pipeline\n", getpid());
			kill(getppid(), SIGINT );						// Sends a SIGINT to parent the if exec fails 
			exit (1);
		}
	}
}


/********************************************************************************/
/*   The function waitPipelineTermination waits for all of the pipeline         */
/*   processes to terminate.                                                    */
/********************************************************************************/

void waitPipelineTermination ()
{
    int status, cpid, i;
    cpid = 0;
    i = 0;
    
    if(pipe_bad)
    {
		raise( SIGINT );		
	}
    
    while(1)
    {
        cpid = wait(&status);					// wait for child to exit
        if(cpid == -1) break;					// If no more children, break
        cmd_status[i] = WEXITSTATUS(status);	// Save the exit status
        fprintf(logfp, "Waiting...process id %d finished\n", cpid);
        fprintf(logfp, "Process id %d finished with exit status %d\n", cpid, cmd_status[i]);
        i++;
    }
}

/********************************************************************************/
/*  This is the signal handler function. It should be called on CNTRL-C signal  */
/*  if any pipeline of processes currently exists.  It will kill all processes  */
/*  in the pipeline, and the piper program will go back to the beginning of the */
/*  control loop, asking for the next pipe command input.                       */
/********************************************************************************/

void killPipeline( int signum )
{
	int i;
	for(i = 0; i < num_cmds; i++)		// Kill all child processes, reduce num_cmds to -1
	{
		if(cmd_pids[i] != 0) kill( cmd_pids[i] , SIGKILL);			
	}
    printf("\n");				// Used to give space after ^C
}

/********************************************************************************/

int main(int ac, char *av[]){

  int i,  pipcount;
  //check usage
  if (ac > 1){
    printf("\nIncorrect use of parameters\n");
    printf("USAGE: %s \n", av[0]);
    exit(1);
  }

  /* Set up signal handler for CNTRL-C to kill only the pipeline processes  */

  logfp =  fopen("LOGFILE", "w");


  while (1) {
     signal(SIGINT, SIG_DFL ); 
     pipcount = 0;

     /*  Get input command file anme form the user */
     char pipeCommand[MAX_INPUT_LINE_LENGTH];

     fflush(stdout);
     printf("Give a list of pipe commands: ");
     gets(pipeCommand); 
     char* terminator = "quit";
     printf("You entered : list of pipe commands  %s\n", pipeCommand);
     if ( strcmp(pipeCommand, terminator) == 0  ) {
        fflush(logfp);
        fclose(logfp);
        printf("Goodbye!\n");
        exit(0);
     }  

    num_cmds = parse_command_line( pipeCommand, cmds);

    /*  SET UP SIGNAL HANDLER  TO HANDLE CNTRL-C                         */
    signal(SIGINT, killPipeline); 

    /*  num_cmds indicates the number of command lines in the input file */

    /* The following code will create a pipeline of processes, one for   */
    /* each command in the given pipe                                    */
    /* For example: for command "ls -l | grep ^d | wc -l "  it will      */
    /* create 3 processes; one to execute "ls -l", second for "grep ^d"  */
    /* and the third for executing "wc -l"                               */
   
	pipe_done = FALSE;		// used by print_info
	pipe_bad = FALSE;		// used to clean up unkilled execs
	for(i=0;i<num_cmds;i++){
         /*  CREATE A NEW PROCCES EXECUTTE THE i'TH COMMAND    */
         /*  YOU WILL NEED TO CREATE A PIPE, AND CONNECT THIS NEW  */
         /*  PROCESS'S stdin AND stdout  TO APPROPRIATE PIPES    */  
         create_command_process (cmds[i], cmd_pids, i);
    }

    print_info(cmds, cmd_pids, cmd_status, num_cmds);

    waitPipelineTermination();

	pipe_done = TRUE;
    print_info(cmds, cmd_pids, cmd_status, num_cmds);
    
    fprintf(logfp, "---------------------------------------------------------------------\n"); // insert a divider between logs

  }
} //end main

/*************************************************/

