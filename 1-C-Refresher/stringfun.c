#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


#define BUFFER_SZ 50

//prototypes
void usage(char *);
void print_buff(char *, int);
int  setup_buff(char *, char *, int);

//prototypes for functions to handle required functionality
int  count_words(char *, int, int);
int reverse_string(char *, int, int);
int word_print(char *, int, int);
//add additional prototypes here


int setup_buff(char *buff, char *user_str, int len){
    char *user_ptr = user_str;      // Pointer to iterate through the user string
    int processed_str_len = 0;      // Length of the processed string
    int whitespace = 0;             // To track consecutive whitespace characters

    // Step 1: Calculate the length of the processed string
    while (*user_ptr != '\0') {
        if (isspace(*user_ptr)) {
            if (whitespace == 0) {
                processed_str_len++;  // Count a single space
                whitespace = 1;
            }
        } else {
            processed_str_len++;      // Count non-space characters
            whitespace = 0;
        }
        user_ptr++;
    }

    // Step 2: Check if the processed string exceeds the buffer length
    if (processed_str_len > len) {
        return -1; // Reject the string
    }

    // Step 3: Reset pointers and copy to the buffer
    user_ptr = user_str;
    char *buff_ptr = buff;          // Pointer to write to the buffer
    int buffer_count = 0;
    whitespace = 0;

    // Trim leading spaces
    while (*user_ptr != '\0' && isspace(*user_ptr)) {
        user_ptr++;
    }

    // Copy characters into the buffer
    while (*user_ptr != '\0') {
        if (isspace(*user_ptr)) {
            if (whitespace == 0) {
                *buff_ptr = ' ';
                buff_ptr++;
                buffer_count++;
                whitespace = 1;
            }
        } else {
            *buff_ptr = *user_ptr;
            buff_ptr++;
            buffer_count++;
            whitespace = 0;
        }
        user_ptr++;
    }

    // Remove trailing space if present
    if (buffer_count > 0 && *(buff_ptr - 1) == ' ') {
        buff_ptr--;
        buffer_count--;
    }

    // Fill remaining buffer space with dots
    while (buffer_count < len) {
        *buff_ptr = '.';
        buff_ptr++;
        buffer_count++;
    }

    return processed_str_len;
}

void print_buff(char *buff, int len){
    printf("Buffer:  ");
    for (int i=0; i<len; i++){
        putchar(*(buff+i));
    }
    putchar('\n');
}

void usage(char *exename){
    printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);

}

int count_words(char *buff, int len, int str_len){
    //YOU MUST IMPLEMENT
    int word_count = 0;
    int same_word = 0; // To track if we are inside a word or not (1 = inside a word, 0 = outside)

    for (int i = 0; i < str_len; i++) {
        if (isspace(buff[i])) {
            same_word = 0;
        } else {
            if (same_word == 0) {
                word_count++;
            } 
            same_word = 1;
        }
    }

    printf("Word Count: %d\n", word_count);
    return word_count;
}

int reverse_string(char *buff, int len, int str_len) {
    int start = 0;
    int end = str_len - 1;

    // Reverse the string in place
    while (start < end) {
        // Swap characters at the start and end
        char temp = buff[start];
        buff[start] = buff[end];
        buff[end] = temp;

        start++;
        end--;
    }

     // Print reversed string without dots
    printf("Reversed String: ");
    for (int i = 0; i < str_len; i++) {
        if (buff[i] == '.') {
            break;  // Stop printing when we encounter a dot
        }
        putchar(buff[i]);  // Print each character
    }
    putchar('\n');  // Print a newline after the reversed string

    return 0;
}

int word_print(char *buff, int len, int str_len) {
    int word_count = 0;
    int same_word = 0;  // Flag to track if we are inside a word
    int start_index = 0;  // To mark the start of a word
    
    printf("Word Print\n");
    printf("----------\n");

    // Iterate over the buffer up to str_len (the length of the user string)
    for (int i = 0; i < str_len; i++) {
        if (isspace(buff[i])) {  // If the character is a space, it's a delimiter
            if (same_word) {  // Word ends
                int word_length = i - start_index;
                printf("%d. ", ++word_count);  // Word number
                for (int j = start_index; j < i; j++) {
                    putchar(buff[j]);  // Print the word
                }
                printf(" (%d)\n", word_length);  // Print the word length
                same_word = 0;  // Reset flag after a word is printed
            }
        } else {
            if (!same_word) {  // Start of a new word
                start_index = i;  // Mark the start of the word
            }
            same_word = 1;  // We are inside a word
        }
    }

    // Handle the last word (if there is one) after the loop
    if (same_word) {
        int word_length = str_len - start_index;
        printf("%d. ", ++word_count);
        for (int i = start_index; i < str_len; i++) {
            putchar(buff[i]);  // Print the last word
        }
        printf(" (%d)\n", word_length);  // Print the word length
    }
    printf("Number of words returned: %d\n", word_count);

    return 0;
}

//ADD OTHER HELPER FUNCTIONS HERE FOR OTHER REQUIRED PROGRAM OPTIONS

int main(int argc, char *argv[]){

    char *buff;             //placehoder for the internal buffer
    char *input_string;     //holds the string provided by the user on cmd line
    char opt;               //used to capture user option from cmd line
    int  rc;                //used for return codes
    int  user_str_len;      //length of user supplied string

    //TODO:  #1. WHY IS THIS SAFE, aka what if arv[1] does not exist?
    //      PLACE A COMMENT BLOCK HERE EXPLAINING
    //      This is safe because the first condition argc < 2 already
    //      make sure that there will be at least 2 args,
    //      so arv[1] will always be valid.
    //      The second condition is only evaluated if the first condition is false
    if ((argc < 2) || (*argv[1] != '-')){
        usage(argv[0]);
        exit(1);
    }

    opt = (char)*(argv[1]+1);   //get the option flag

    //handle the help flag and then exit normally
    if (opt == 'h'){
        usage(argv[0]);
        exit(0);
    }

    //WE NOW WILL HANDLE THE REQUIRED OPERATIONS

    //TODO:  #2 Document the purpose of the if statement below
    //      PLACE A COMMENT BLOCK HERE EXPLAINING
    //      This means that if there are only two args which is ./stringfun
    //      and the option flag but missing the additional arg, 
    //      then its the wrong usage of the command.
    //      So it will be similar to the way the help flag is handled which is
    //      to remind the user the proper usage of the command
    if (argc < 3){
        usage(argv[0]);
        exit(1);
    }

    input_string = argv[2]; //capture the user input string

    //TODO:  #3 Allocate space for the buffer using malloc and
    //          handle error if malloc fails by exiting with a 
    //          return code of 99
    // CODE GOES HERE FOR #3
    buff = malloc(BUFFER_SZ);
    if (buff == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(99); 
    }



    user_str_len = setup_buff(buff, input_string, BUFFER_SZ);     //see todos
    if (user_str_len < 0){
        printf("Error setting up buffer, error = %d", user_str_len);
        exit(2);
    }

    switch (opt){
        case 'c':
            rc = count_words(buff, BUFFER_SZ, user_str_len);  //you need to implement
            if (rc < 0){
                printf("Error counting words, rc = %d", rc);
                exit(2);
            }
            break;
        case 'r':
            rc = reverse_string(buff, BUFFER_SZ, user_str_len);
            if (rc < 0){
                printf("Error reversing string, rc = %d", rc);
                exit(2);
            }
            break;
        case 'w':
            rc = word_print(buff, BUFFER_SZ, user_str_len);
            if (rc < 0){
                printf("Error printing words, rc = %d", rc);
                exit(2);
            }
            break;

        //TODO:  #5 Implement the other cases for 'r' and 'w' by extending
        //       the case statement options
        default:
            usage(argv[0]);
            exit(1);
    }

    //TODO:  #6 Dont forget to free your buffer before exiting
    free(buff);
    print_buff(buff,BUFFER_SZ);
    exit(0);
}

//TODO:  #7  Notice all of the helper functions provided in the 
//          starter take both the buffer as well as the length.  Why
//          do you think providing both the pointer and the length
//          is a good practice, after all we know from main() that 
//          the buff variable will have exactly 50 bytes?
//  
//          Its good practice to ensure that when working in function,
//          you dont access memory out of bounds, preventing potential segmentation faults.
//          In some functions, the buffer length are not even needed.