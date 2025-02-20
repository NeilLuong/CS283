#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "dshlib.h"
#include <errno.h>

/*
 * Implement your exec_local_cmd_loop function by building a loop that prompts the 
 * user for input.  Use the SH_PROMPT constant from dshlib.h and then
 * use fgets to accept user input.
 * 
 *      while(1){
 *        printf("%s", SH_PROMPT);
 *        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL){
 *           printf("\n");
 *           break;
 *        }
 *        //remove the trailing \n from cmd_buff
 *        cmd_buff[strcspn(cmd_buff,"\n")] = '\0';
 * 
 *        //IMPLEMENT THE REST OF THE REQUIREMENTS
 *      }
 * 
 *   Also, use the constants in the dshlib.h in this code.  
 *      SH_CMD_MAX              maximum buffer size for user input
 *      EXIT_CMD                constant that terminates the dsh program
 *      SH_PROMPT               the shell prompt
 *      OK                      the command was parsed properly
 *      WARN_NO_CMDS            the user command was empty
 *      ERR_TOO_MANY_COMMANDS   too many pipes used
 *      ERR_MEMORY              dynamic memory management failure
 * 
 *   errors returned
 *      OK                     No error
 *      ERR_MEMORY             Dynamic memory management failure
 *      WARN_NO_CMDS           No commands parsed
 *      ERR_TOO_MANY_COMMANDS  too many pipes used
 *   
 *   console messages
 *      CMD_WARN_NO_CMD        print on WARN_NO_CMDS
 *      CMD_ERR_PIPE_LIMIT     print on ERR_TOO_MANY_COMMANDS
 *      CMD_ERR_EXECUTE        print on execution failure of external command
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 1+)
 *      malloc(), free(), strlen(), fgets(), strcspn(), printf()
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 2+)
 *      fork(), execvp(), exit(), chdir()
 */
int last_rc = 0;    //global variable to store the return code 

int exec_local_cmd_loop()
{
    char *cmd_buff;
    int rc = 0;
    cmd_buff_t cmd;

    // TODO IMPLEMENT MAIN LOOP
    
    // Allocate memory for the comd buffer
    if ((rc = alloc_cmd_buff(&cmd)) != OK) {
        fprintf(stderr, "Memory allocation failed\n");
        return ERR_MEMORY;
    }


    while (1) {
        printf("%s", SH_PROMPT);

        if (fgets(cmd._cmd_buffer, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }

        // Remove the trailing newline
        cmd._cmd_buffer[strcspn(cmd._cmd_buffer, "\n")] = '\0';

        // Warning if input is empty
        if (cmd._cmd_buffer[0] == '\0') {
            printf("%s\n", CMD_WARN_NO_CMD);
            continue;
        }


        // Trim any leading whitespace from the command
        char *trimmed = cmd._cmd_buffer;
        while (*trimmed && isspace((unsigned char)*trimmed))
            trimmed++;
        // If the trimmed input is empty, warn the user and continue
        if (*trimmed == '\0') {
            printf("%s\n", CMD_WARN_NO_CMD);
            continue;
        }

        // "exit" command
        if (strcmp(cmd._cmd_buffer, EXIT_CMD) == 0)
            break;

        // "dragon" command
        if (strcmp(cmd._cmd_buffer, "dragon") == 0) {
            print_dragon();
            continue;
        }
        
        // "rc" command
        if (strcmp(trimmed, "rc") == 0) {
            printf("%d\n", last_rc);
            continue;
        }

        // Parse the command line into tokens.
        if ((rc = build_cmd_buff(cmd._cmd_buffer, &cmd)) != OK) {
            fprintf(stderr, "Error parsing command\n");
            continue;
        }

        // Handle the "cd" built-in.
        if (strcmp(cmd.argv[0], "cd") == 0) {
            handle_cd(cmd.argv[1]);
            continue;
        }

        // Fork a child process to execute external commands
        pid_t pid = fork();
        if (pid < 0) {
            if (errno == EAGAIN) {
                fprintf(stderr, "Error: fork() failed because the system process limit has been reached (EAGAIN).\n");
            } else if (errno == ENOMEM) {
                fprintf(stderr, "Error: fork() failed due to insufficient memory (ENOMEM).\n");
            } else {
                perror("fork");
            }
            continue;
        }

        if (pid == 0) {
            // Child process: execute the command.
            execvp(cmd.argv[0], cmd.argv);
            // If execvp returns, then an error occurred
            int err = errno;
            switch (err) {
                case ENOENT:
                    fprintf(stderr, "Command not found in PATH\n");
                    break;
                case EACCES:
                    fprintf(stderr, "Permission denied\n");
                    break;
                default:
                    fprintf(stderr, "Execution failed: %s\n", strerror(err));
                    break;
            }
            exit(err);  // Return errno as the exit code.
        } else {
            // Parent process: wait for the child.
            int status;
            waitpid(pid, &status, 0);
            last_rc = WEXITSTATUS(status);
            // Do not print extra exit-status messages.
        }
    }

    free_cmd_buff(&cmd); // Free allocated memory
    return OK;
}

// Changes the current working directory to the provided path. If no path is provided, it does nothing
int handle_cd(char *path) {
    if (path == NULL)
        return 0;
    if (chdir(path) != 0) {
        perror("cd");
        return -1;
    }
    return 0;
}


int alloc_cmd_buff(cmd_buff_t *cmd) {
    if (cmd == NULL) return ERR_MEMORY;
    cmd->_cmd_buffer = malloc(SH_CMD_MAX + 1); // +1 for the null terminator
    if (cmd->_cmd_buffer == NULL) return ERR_MEMORY;
    return OK;
}

int free_cmd_buff(cmd_buff_t *cmd) {
    if (cmd == NULL) return ERR_MEMORY;
    free(cmd->_cmd_buffer);
    cmd->_cmd_buffer = NULL;
    return OK;
}

int clear_cmd_buff(cmd_buff_t *cmd) {
    if (cmd == NULL) return ERR_MEMORY;
    cmd->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd->argv[i] = NULL;
    }
    return OK;
}

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd) {
    if (cmd_line == NULL || cmd == NULL) return ERR_MEMORY;
    
    // Clear any previous command data.
    clear_cmd_buff(cmd);
    int count = 0;
    char *p = cmd_line;
    
    while (*p) {
        // Skip any leading whitespace.
        while (*p && isspace((unsigned char)*p))
            p++;
        if (!*p)
            break;
        
        char *token_start = p;
        
        if (*p == '"' || *p == '\'') {
            // If the token starts with a quote, skip the quote
            char quote = *p;
            p++;
            token_start = p;  // Token starts after the opening quote
            // Move until the closing matching quote is found
            while (*p && *p != quote)
                p++;
            // Terminate the token at the closing quote
            if (*p == quote) {
                *p = '\0';
                p++;
            }
        } else {
            // For unquoted tokens, read until the next whitespace
            while (*p && !isspace((unsigned char)*p))
                p++;
            if (*p) {
                *p = '\0';
                p++;
            }
        }
        // Store the token in the argv array.
        cmd->argv[count++] = token_start;
        // Prevent overflow by stopping before reaching CMD_ARGV_MAX
        if (count >= CMD_ARGV_MAX - 1)
            break;
    }
    // Null-terminate the argv array.
    cmd->argv[count] = NULL;
    cmd->argc = count;
    
    // If no tokens were parsed, return a warning.
    if (count == 0)
        return WARN_NO_CMDS;
    
    return OK;
}
