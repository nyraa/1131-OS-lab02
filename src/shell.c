#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "../include/command.h"
#include "../include/builtin.h"

#define PROMPT ">>> \xE0\xB6\x9E "

// ======================= requirement 2.3 =======================
/**
 * @brief 
 * Redirect command's stdin and stdout to the specified file descriptor
 * If you want to implement ( < , > ), use "in_file" and "out_file" included the cmd_node structure
 * If you want to implement ( | ), use "in" and "out" included the cmd_node structure.
 *
 * @param p cmd_node structure
 * 
 */
void redirection(struct cmd_node *p)
{
	if(p->in_file)
	{
		int fd = open(p->in_file, O_RDONLY);
		if(fd == -1)
		{
			perror("open");
			exit(1);
		}
		dup2(fd, STDIN_FILENO);
		close(fd);
	}
	if(p->out_file)
	{
		int fd = open(p->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if(fd == -1)
		{
			perror("open");
			exit(1);
		}
		dup2(fd, STDOUT_FILENO);
		close(fd);
	}
}
// ===============================================================

// ======================= requirement 2.2 =======================
/**
 * @brief 
 * Execute external command
 * The external command is mainly divided into the following two steps:
 * 1. Call "fork()" to create child process
 * 2. Call "execvp()" to execute the corresponding executable file
 * @param p cmd_node structure
 * @return int 
 * Return execution status
 */
int spawn_proc(struct cmd_node *p)
{
	pid_t cpid = fork();
	switch(cpid)
	{
		case -1:
			perror("fork");
			return 0;
		case 0:
			// child process
			redirection(p);
			if(p->in != STDIN_FILENO)
			{
				dup2(p->in, STDIN_FILENO);
				close(p->in);
			}
			if(p->out != STDOUT_FILENO)
			{
				dup2(p->out, STDOUT_FILENO);
				close(p->out);
			}
			if(execvp(p->args[0], p->args) == -1)
			{
				perror("execvp");
				return 0;
			}
			break;
		default:
			// parent process
			return cpid;
	}
  	return 1;
}
// ===============================================================


// ======================= requirement 2.4 =======================
/**
 * @brief 
 * Use "pipe()" to create a communication bridge between processes
 * Call "spawn_proc()" in order according to the number of cmd_node
 * @param cmd Command structure  
 * @return int
 * Return execution status 
 */
int fork_cmd_node(struct cmd *cmd)
{
	int pipefd[2];
	int last_read = -1;	// init last_read to -1
	int *cpids = (int *)malloc(sizeof(int) * cmd->pipe_num);
	// loop through cmd_node
	int i = 0;
	for(struct cmd_node *temp = cmd->head; temp != NULL; temp = temp->next)
	{
		// STEP 1: if node is not first node, set in to pipefd[0] (previous pipe read end)
		if(temp != cmd->head)
		{
			// printf("%s form pipe in\n", temp->args[0]);
			temp->in = pipefd[0];	// pipefd[0] is read end
			last_read = pipefd[0];	// tmp for previous pipe read end, should close after fork, but before fork new pipe creates, so store it
		}

		// STEP 2: if next exists, create pipe
		if(temp->next != NULL)
		{
			if(pipe(pipefd) == -1)
			{
				perror("pipe");
				exit(1);
			}
			temp->out = pipefd[1];	// pipefd[1] is write end
		}

		// STEP 3: fork process
		cpids[i] = spawn_proc(temp);
		// close pipefd[1] if not last node
		if(temp->next != NULL)
		{
			close(pipefd[1]);
		}
		// close last_read if not first node
		if(last_read != -1)
		{
			close(last_read);
		}
		i += 1;
	}
	// wait for all child processes
	for(int j = 0; j < i; j++)
	{
		waitpid(cpids[j], NULL, 0);
	}
	free(cpids);
	return 1;
}
// ===============================================================


void shell()
{
	while (1) {
		printf(PROMPT);
		char *buffer = read_line();
		if (buffer == NULL)
			continue;

		struct cmd *cmd = split_line(buffer);
		
		int status = -1;
		// only a single command
		struct cmd_node *temp = cmd->head;
		
		if(temp->next == NULL){
			status = searchBuiltInCommand(temp);
			if (status != -1){
				int in = dup(STDIN_FILENO), out = dup(STDOUT_FILENO);
				if( in == -1 || out == -1)
					perror("dup");
				redirection(temp);
				status = execBuiltInCommand(status,temp);

				// recover shell stdin and stdout
				if (temp->in_file)
				{
					dup2(in, STDIN_FILENO);
				}
				if (temp->out_file)
				{
					dup2(out, STDOUT_FILENO);
				}
				close(in);
				close(out);
			}
			else{
				//external command
				status = spawn_proc(cmd->head);
				waitpid(status, NULL, 0);
			}
		}
		// There are multiple commands ( | )
		else{
			// for(struct cmd_node *temp = cmd->head; temp != NULL; temp = temp->next)
			// {
			// 	printf("args: %s, out: %d\n", temp->args[0], temp->out);
			// }
			status = fork_cmd_node(cmd);
		}
		// free space
		while (cmd->head) {
			
			struct cmd_node *temp = cmd->head;
      		cmd->head = cmd->head->next;
			free(temp->args);
   	    	free(temp);
   		}
		free(cmd);
		free(buffer);
		
		if (status == 0)
			break;
	}
}
