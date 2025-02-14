#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

/*
 *  build_cmd_list
 *    cmd_line:     the command line from the user
 *    clist *:      pointer to clist structure to be populated
 *
 *  This function builds the command_list_t structure passed by the caller
 *  It does this by first splitting the cmd_line into commands by spltting
 *  the string based on any pipe characters '|'.  It then traverses each
 *  command.  For each command (a substring of cmd_line), it then parses
 *  that command by taking the first token as the executable name, and
 *  then the remaining tokens as the arguments.
 *
 *  NOTE your implementation should be able to handle properly removing
 *  leading and trailing spaces!
 *
 *  errors returned:
 *
 *    OK:                      No Error
 *    ERR_TOO_MANY_COMMANDS:   There is a limit of CMD_MAX (see dshlib.h)
 *                             commands.
 *    ERR_CMD_OR_ARGS_TOO_BIG: One of the commands provided by the user
 *                             was larger than allowed, either the
 *                             executable name, or the arg string.
 *
 *  Standard Library Functions You Might Want To Consider Using
 *      memset(), strcmp(), strcpy(), strtok(), strlen(), strchr()
 */
int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    // Check if the command line is empty or NULL
    if (cmd_line == NULL || cmd_line[0] == '\0') {
        return WARN_NO_CMDS;  // No command entered
    }

    // Initialize the command list structure with zeros
    memset(clist, 0, sizeof(command_list_t));

    // Count the number of pipes '|' in the input command line
    int pipe_count = 0;
    for (char *c = cmd_line; *c != '\0'; c++) {
        if (*c == PIPE_CHAR) pipe_count++;
    }

    // Ensure the number of commands does not exceed the maximum allowed
    if (pipe_count >= CMD_MAX) {
        return ERR_TOO_MANY_COMMANDS;
    }

    // Tokenize the input string using pipes as delimiters
    char *cmd_token = strtok(cmd_line, PIPE_STRING);

    while (cmd_token != NULL) {
        // Ensure the command count does not exceed the allowed maximum
        if (clist->num >= CMD_MAX) {
            return ERR_TOO_MANY_COMMANDS;
        }

        // Trim leading spaces
        while (*cmd_token == SPACE_CHAR) cmd_token++;

        // Trim trailing spaces
        int len = strlen(cmd_token);
        while (len > 0 && cmd_token[len - 1] == SPACE_CHAR) {
            cmd_token[--len] = '\0';
        }

        // If the token is empty, it means there's a pipe with no command
        if (strlen(cmd_token) == 0) {
            return WARN_NO_CMDS;
        }

        // Split the command into executable name and arguments
        char *arg_token = strchr(cmd_token, SPACE_CHAR);
        if (arg_token != NULL) {
            *arg_token = '\0';  // Terminate the executable name string
            arg_token++;  // Move to the start of the arguments

            // Ensure the argument length does not exceed the maximum allowed
            if (strlen(arg_token) >= ARG_MAX) {
                return ERR_CMD_OR_ARGS_TOO_BIG;
            }

            // Copy the arguments into the command structure
            strncpy(clist->commands[clist->num].args, arg_token, ARG_MAX - 1);
            clist->commands[clist->num].args[ARG_MAX - 1] = '\0';  // Null-terminate
        } else {
            // No arguments, so set args to an empty string
            clist->commands[clist->num].args[0] = '\0';
        }

        // Ensure the executable name does not exceed the maximum allowed length
        if (strlen(cmd_token) >= EXE_MAX) {
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }

        // Copy the executable name into the command structure
        strncpy(clist->commands[clist->num].exe, cmd_token, EXE_MAX - 1);
        clist->commands[clist->num].exe[EXE_MAX - 1] = '\0';  // Null-terminate

        // Increment the command count
        clist->num++;
        
        // Get the next command token
        cmd_token = strtok(NULL, PIPE_STRING);
    }

    return OK;  // Successfully parsed the command list
}
