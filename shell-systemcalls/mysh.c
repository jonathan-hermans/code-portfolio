/*******************************************
 *
 *               mysh.c
 *
 *  Jonathan Hermans - 100642234
 *  CS3310U - Systems Programming
 *  2020-02-02
 ****************************************/

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#define CMD_ERR 0
#define CMD_SCS 1
#define ERR_EXT 2
#define ERR_SYS 3
#define PAR_MIA 4
#define FRK_ERR 5
#define FIL_ERR 6
#define OUT_ERR 7
#define IND_ERR 8
#define PIP_ERR 9
#define PIP_MIA 10

#define BUFFER_SIZE 512

void terminate_string(int length, char argv[BUFFER_SIZE]);
void command_exit();
void command_prompt();
void error_check();
void execute_command(int count);
int split_strings(char command[BUFFER_SIZE]);
int check_pipe_command(int start_command, int string_number);
int check_redirect_command(int start_command, int redirect_number);

const char CONST_CMT = '#';
const char CONST_NULLC = '\0';
const char CONST_SPACE = ' ';
const char* CONST_PIPE = "|";
const char* CONST_OUT = ">";
const char* CONST_IN = "<";
const char* CONST_SH = "sh";
const char* CONST_EXIT = "exit";
const char* CONST_PROMPT = "prompt";

int status_code = 0;

char user_prompt[BUFFER_SIZE];
char user_commands[BUFFER_SIZE][BUFFER_SIZE];

int cmd_count = 0;

int main() 
{
	char argv[BUFFER_SIZE];
	
	while (1) 
	{	
		printf("\e[34;1m%s~ ", user_prompt);
		fgets(argv, BUFFER_SIZE, stdin);
		
		// check for comments
		char* hash = strchr(argv, CONST_CMT);
		if (hash != NULL)
		{
			*hash = CONST_NULLC;
		}
			
		int length = strlen(argv);
		
		terminate_string(length, argv);
		cmd_count = split_strings(argv);

		// check for command type
		if (cmd_count != 0) 
		{ 
			if (strcmp(user_commands[0], CONST_EXIT) == 0) 
			{ 
				command_exit();		
			} 
			else if (strcmp(user_commands[0], CONST_PROMPT) == 0) 
			{
				command_prompt();
			}
			else 
			{ 
			    execute_command(cmd_count);
			    error_check();
			    if (status_code == CMD_SCS && strcmp(user_commands[0], CONST_SH) == 0)
				{
					command_exit();
				}									
			}
		}
	}
	return 0;
}

void terminate_string(int length, char argv[BUFFER_SIZE])
{
	if (length != BUFFER_SIZE) 
	{
		argv[length-1] = '\0';
	}	
}

void command_exit() 
{ 
	pid_t pid = getpid();
	kill(pid, SIGTERM);
}

void command_prompt() 
{
	strcpy(user_prompt, user_commands[1]);
}

void execute_command(int count) 
{
	// fork and check process id 
	pid_t pid = fork();
	
	if (pid < 0) 
	{
		status_code = FRK_ERR;
	} 
	else if (pid == 0) 
	{
		int in_file_descr = dup(fileno(stdin));
		int out_file_descr = dup(fileno(stdout));
		// follows through and checks redirect
		int status_code = check_pipe_command(0, count);
		
		dup2(in_file_descr, fileno(stdin));
		dup2(out_file_descr, fileno(stdout));
		exit(status_code);
	} 
	else 
	{
		int status;
		waitpid(pid, &status, 0);
		status_code = WEXITSTATUS(status);
	}
}

void error_check()
{
	if (status_code == FRK_ERR)
	{
		fprintf(stderr, "\e[34;1mError: Fork error.\n");
		exit(FRK_ERR);					
	}
	else if (status_code == CMD_ERR)
	{
		fprintf(stderr, "\e[34;1mError: Command not recognized.\n");
	}		
	else if (status_code == OUT_ERR)
	{
		fprintf(stderr, "\e[34;1mError: Redirection error.\n");
	}	
	else if (status_code == IND_ERR)
	{
		fprintf(stderr, "\e[34;1mError: Redirection error.\n");
	}				
	else if (status_code == FIL_ERR)
	{
		fprintf(stderr, "\e[34;1mError: File does not exist.\n");
	}	
	else if (status_code == PAR_MIA)
	{
		fprintf(stderr, "\e[34;1mError: Expected parameters (e.g a file).\n");
	}	
	else if (status_code == PIP_ERR)
	{
		fprintf(stderr, "\e[34;1mError: Pipe error.\n");
	}					
	else if (status_code == PIP_MIA)
	{
		fprintf(stderr, "\e[34;1mError: Pipe needs to be pointed to a program.\n");
	}
}

int check_pipe_command(int start_command, int string_number) 
{ 	
	int pipe_index = -1;
	int file_descr[2];
	int status_code = CMD_SCS;
	
	// from the start to the end of the number of commands right of pipe	
	for (int i = start_command; i < string_number; i++) 
	{
		// find pipe in commands then break out of loop
		if (strcmp(user_commands[i], CONST_PIPE) == 0) 
		{			
			pipe_index = i;
			break;
		}
	}
	
	// if pipe isn't found, look for redirection
	if (pipe_index == -1) 
	{ 
		return check_redirect_command(start_command, string_number);
	}
	// if there is a command missing right of the pipe
	else if (pipe_index + 1 == string_number) 
	{ 
		return PIP_MIA;
	}
	
	// linux pipe command result
	if (pipe(file_descr) == -1) 
	{
		return PIP_ERR;
	}

	// pipeline
	pid_t pid = vfork();

	if (pid < 0) 
	{
		status_code = FRK_ERR;
	} 
	else if (pid == 0) 
	{ 
		close(file_descr[0]);
		dup2(file_descr[1], fileno(stdout)); 
		close(file_descr[1]);
		
		status_code = check_redirect_command(start_command, pipe_index);
		exit(status_code);
	} 
	else 
	{ 
		int status;
		waitpid(pid, &status, 0);
		int exit_code = WEXITSTATUS(status);
		
		if (exit_code != CMD_SCS) 
		{ 
			status_code = exit_code;
		} 
		// recursive
		else if (pipe_index + 1 < string_number)
		{			
			close(file_descr[1]);
			dup2(file_descr[0], fileno(stdin));
			close(file_descr[0]);
			status_code = check_pipe_command(pipe_index + 1, string_number);
		}
	}
	return status_code;
}

int check_redirect_command(int start_command, int string_number) 
{ 
	int indirection_count = 0;
	int redirection_count = 0;
	int status = 0;
	
	char* in_file = NULL;
	char* out_file = NULL;
	char* ptr_commands[BUFFER_SIZE];
	
	int status_code = CMD_SCS;	
	// redirect_index will be located at the command it is found
	// currently at the end of all strings
	int redirect_index = string_number; 

	// count how many indirection or redirection signs there are
	for (int i = start_command; i < string_number; i++) 
	{
		// inner direction
		if (strcmp(user_commands[i], CONST_IN) == 0) 
		{
			// found a symbol
			indirection_count++;
			
			// check that symbol + 1 is less than the total number of strings
			// string is located beside the symbol hence +1
			// save the string as in_file			
			if (i + 1 < string_number)
			{
				in_file = user_commands[i+1];	
			}
			// early exit if no string is beside the symbol		
			else
			{
				return PAR_MIA; 	
			} 

			if (redirect_index == string_number)
			{
				// set the redirect_index to the starting string
			    redirect_index = i;	
			} 
		} 
		// outer direction
		else if (strcmp(user_commands[i], CONST_OUT) == 0) 
		{ 
			// redirection count
			redirection_count++;
			if (i + 1 < string_number)
			{
				out_file = user_commands[i+1];				
			}
			// early exit
			else 
			{
				return PAR_MIA; 
			}

			// now go from the start			
			if (redirect_index == string_number)
			{
				redirect_index = i;	
			} 
		}
	}

	// if the is an inner redirection	
	if (indirection_count == 1) 
	{
		// file must exist
		FILE* fp = fopen(in_file, "r");
		
		// check if file pointer is null 	
		if (fp == NULL) 
		{
			return FIL_ERR;	
		}
		
		fclose(fp);
	}

	// error check
	// if there is more than one	
	if (indirection_count > 1) 
	{
		return IND_ERR;
	} 
	else if (redirection_count > 1) 
	{ 
		return OUT_ERR;
	}

	// block parent
	pid_t pid = vfork();
	
	if (pid < 0) 
	{
		status_code = FRK_ERR;
	} 
	else if (pid == 0) 
	{
		// read
		if (indirection_count == 1)
		{
			freopen(in_file, "r", stdin);			
		}
		// write
		if (redirection_count == 1)
		{
			freopen(out_file, "w", stdout);			
		}
		
		// prepare pointer of commands for execvp  
		for (int i = start_command; i < redirect_index; i++)
		{
			ptr_commands[i] = user_commands[i];		
		}
		
		ptr_commands[redirect_index] = NULL;
		execvp(ptr_commands[start_command], ptr_commands + start_command);
		exit(errno); 
	} 
	else 
	{
		waitpid(pid, &status, 0);
		int error = WEXITSTATUS(status); 

		if (error) 
		{ 
			printf("\e[34;1mParent, pid %d. Child pid is %d. Error is: %s\n", getppid(), pid, strerror(error));
		}
	}
	return status_code;
}

int split_strings(char argv[BUFFER_SIZE]) 
{
	//int count_cmds = 0;
	int j = 0;
	int length = strlen(argv);
	int cnr = 0;
	char **cmd_tok = malloc(BUFFER_SIZE*sizeof(char*));	
	char **file_tok = malloc(BUFFER_SIZE*sizeof(char*));	


	char **cmd_tok2 = malloc(BUFFER_SIZE*sizeof(char*));	
	char **file_tok2 = malloc(BUFFER_SIZE*sizeof(char*));	

	// take argv apart and fit it into user_commands array
	for (int i = 0; i < length; i++) 
	{	
	    // if not a space
		if (argv[i] != ' ') 
		{
			user_commands[cmd_count][j++] = argv[i];
		} 
		else 
		{
			// add null character
			if (j != 0) 
			{
				user_commands[cmd_count][j] = '\0';
				cmd_count++;
				j = 0;
			}
		}
	}
	
	// add null character	
	if (j != 0) 
	{
		user_commands[count_cmds][j] = '\0';
		cmd_count++;
	}
	
	for (int i = 0; i < cmd_count; i++)
	{
		if (strcmp(user_commands[i], CONST_OUT) == 0)
		{
			for (char *tokout = strtok(user_commands, CONST_OUT); tokout != NULL; tokout = strtok(NULL, CONST_OUT))
			{				
				if (!cnr % 2)
				{
					cmd_tok[cnr] = tokout;
					printf("This is a cmd token %s \n", cmd_tok[cnr]);
					user_commands[cnr][j++] = cmd_tok[cnr];							
				}
				else
				{
					file_tok[cnr] = tokout;
					file_tok[cnr] += CONST_NULLC;
					printf("This is a file token %s \n", file_tok[cnr]);
					user_commands[cnr][j++] = file_tok[cnr];															
				}
				cnr += 1;
			}				
		}
		else if (user_commands[i] == '<')
		{
			for (char *tokout = strtok(user_commands, CONST_IN); tokout != NULL; tokout = strtok(NULL, CONST_IN))
			{				
				if (!cnr % 2)
				{
					cmd_tok2[cnr] = tokout;
					printf("This is a cmd token %s \n", cmd_tok2[cnr]);
					user_commands[cnr][j++] = cmd_tok2[cnr];							
				}
				else
				{
					file_tok2[cnr] = tokout;
					//file_tok2[cnr] += CONST_NULLC;
					printf("This is a file token %s \n", file_tok2[cnr]);
					user_commands[cnr][j++] = file_tok2[cnr];															
				}
				cnr += 1;
			}				
		}
		else if (strcmp(user_commands[i], CONST_SPACE) == 0)
		{
			// add null character
			if (j != 0) 
			{
				user_commands[cnr][j] = '\0';
				cnr++;
				j = 0;
			}
		} 
	}
	
	
	
	return cmd_count;
}
