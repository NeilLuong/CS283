#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include "dshlib.h"

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

 int last_rc = 0;

int execute_pipeline(command_list_t *clist) {
    if (clist->num == 0) {
        return OK;
    }
    
    // Handle built-in commands when they are the only command
    if (clist->num == 1) {
        // Handle "exit" command
        if (clist->commands[0].argc > 0 && 
            strcmp(clist->commands[0].argv[0], EXIT_CMD) == 0) {
            return OK_EXIT;
        }
        
        // Handle "cd" command
        if (clist->commands[0].argc > 0 && 
            strcmp(clist->commands[0].argv[0], "cd") == 0) {
            handle_cd(clist->commands[0].argc > 1 ? clist->commands[0].argv[1] : NULL);
            return OK;
        }
        
        // Handle "rc" command
        if (clist->commands[0].argc > 0 && 
            strcmp(clist->commands[0].argv[0], "rc") == 0) {
            printf("%d\n", last_rc);
            return OK;
        }
        
    }
    
    // Allocate array to store child PIDs
    pid_t *child_pids = malloc(clist->num * sizeof(pid_t));
    if (child_pids == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return ERR_MEMORY;
    }
    
    // Array to store pipe file descriptors
    int pipes[CMD_MAX - 1][2]; // One less pipe than maximum commands
    
    // Create all necessary pipes (one less than number of commands)
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            // Close any previously created pipes
            for (int j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            free(child_pids);
            return ERR_EXEC_CMD;
        }
    }
    
    // Execute each command in the pipeline
    for (int i = 0; i < clist->num; i++) {
        // Fork a child process
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            
            // Close all pipes
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            free(child_pids);
            return ERR_EXEC_CMD;
        }
        
        if (pid == 0) {
            // Child process
            
            // Set up input from previous pipe (if not first command)
            if (i > 0) {
                if (dup2(pipes[i-1][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(ERR_EXEC_CMD);
                }
            }
            
            // Set up output to next pipe (if not last command)
            if (i < clist->num - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(ERR_EXEC_CMD);
                }
            }
            
            // Close all pipe file descriptors
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Execute the command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            
            // If execvp returns, an error occurred
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
            exit(err);
        } else {
            // Parent process
            child_pids[i] = pid;
        }
    }
    
    // Close all pipe file descriptors in parent
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all child processes
    int last_status = 0;
    for (int i = 0; i < clist->num; i++) {
        int status;
        waitpid(child_pids[i], &status, 0);
        
        // Save the exit status of the last command
        if (i == clist->num - 1) {
            last_status = WEXITSTATUS(status);
            last_rc = last_status;
        }
    }
    
    free(child_pids);
    return OK;
}

int build_cmd_list(char *cmd_line, command_list_t *clist) {
    char *cmd_copy = strdup(cmd_line);
    if (cmd_copy == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return ERR_MEMORY;
    }
    
    // Initialize command list
    clist->num = 0;
    
    // Split command by pipe characters
    char *token;
    char *saveptr;
    token = strtok_r(cmd_copy, PIPE_STRING, &saveptr);
    
    while (token != NULL && clist->num < CMD_MAX) {
        // Allocate memory for the command buffer
        if (alloc_cmd_buff(&clist->commands[clist->num]) != OK) {
            free(cmd_copy);
            // Free previously allocated command buffers
            for (int i = 0; i < clist->num; i++) {
                free_cmd_buff(&clist->commands[i]);
            }
            return ERR_MEMORY;
        }
        
        // Copy token to command buffer for parsing
        strncpy(clist->commands[clist->num]._cmd_buffer, token, SH_CMD_MAX);
        clist->commands[clist->num]._cmd_buffer[SH_CMD_MAX] = '\0';
        
        // Parse the command
        if (build_cmd_buff(clist->commands[clist->num]._cmd_buffer, &clist->commands[clist->num]) != OK) {
            free_cmd_buff(&clist->commands[clist->num]);
            free(cmd_copy);
            // Free previously allocated command buffers
            for (int i = 0; i < clist->num; i++) {
                free_cmd_buff(&clist->commands[i]);
            }
            return ERR_CMD_ARGS_BAD;
        }
        
        // Increment command count
        clist->num++;
        
        // Get next token
        token = strtok_r(NULL, PIPE_STRING, &saveptr);
    }
    
    free(cmd_copy);
    
    // Check if we exceeded the command limit
    if (token != NULL && clist->num >= CMD_MAX) {
        fprintf(stderr, CMD_ERR_PIPE_LIMIT, CMD_MAX);
        // Free allocated command buffers
        for (int i = 0; i < clist->num; i++) {
            free_cmd_buff(&clist->commands[i]);
        }
        return ERR_TOO_MANY_COMMANDS;
    }
    
    // Check if any commands were parsed
    if (clist->num == 0) {
        fprintf(stderr, CMD_WARN_NO_CMD);
        return WARN_NO_CMDS;
    }
    
    return OK;
}

int free_cmd_list(command_list_t *clist) {
    if (clist == NULL) {
        return ERR_MEMORY;
    }
    
    for (int i = 0; i < clist->num; i++) {
        free_cmd_buff(&clist->commands[i]);
    }
    
    clist->num = 0;
    return OK;
}

int exec_local_cmd_loop() {
    int rc = 0;
    cmd_buff_t cmd;
    command_list_t cmd_list;

    // Allocate memory for the command buffer
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

        // Skip empty commands
        char *trimmed = cmd._cmd_buffer;
        while (*trimmed && isspace((unsigned char)*trimmed))
            trimmed++;
            
        if (*trimmed == '\0') {
            printf("%s", CMD_WARN_NO_CMD);
            continue;
        }

        // Handle exit command directly
        if (strcmp(trimmed, EXIT_CMD) == 0)
            break;

        if (strcmp(cmd._cmd_buffer, "dragon") == 0) {
            print_dragon();
            continue;
        }

        // Parse the command line into a pipeline
        rc = build_cmd_list(trimmed, &cmd_list);
        if (rc == WARN_NO_CMDS) {
            continue;
        } else if (rc != OK) {
            fprintf(stderr, "Error parsing command: %d\n", rc);
            continue;
        }

        // Execute the pipeline
        rc = execute_pipeline(&cmd_list);
        
        // Free the command list
        free_cmd_list(&cmd_list);
        
        // Exit if requested by built-in command
        if (rc == OK_EXIT) {
            break;
        }
    }

    free_cmd_buff(&cmd);
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
