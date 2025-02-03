#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> //c library for system call file routines
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>

// database include files
#include "db.h"
#include "sdbsc.h"

/*
 *  open_db
 *      dbFile:  name of the database file
 *      should_truncate:  indicates if opening the file also empties it
 *
 *  returns:  File descriptor on success, or ERR_DB_FILE on failure
 *
 *  console:  Does not produce any console I/O on success
 *            M_ERR_DB_OPEN on error
 *
 */
int open_db(char *dbFile, bool should_truncate)
{
    // Set permissions: rw-rw----
    // see sys/stat.h for constants
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

    // open the file if it exists for Read and Write,
    // create it if it does not exist
    int flags = O_RDWR | O_CREAT;

    if (should_truncate)
        flags += O_TRUNC;

    // Now open file
    int fd = open(dbFile, flags, mode);

    if (fd == -1)
    {
        // Handle the error
        printf(M_ERR_DB_OPEN);
        return ERR_DB_FILE;
    }

    return fd;
}

/*
 *  get_student
 *      fd:  linux file descriptor
 *      id:  the student id we are looking forname of the
 *      *s:  a pointer where the located (if found) student data will be
 *           copied
 *
 *  returns:  NO_ERROR       student located and copied into *s
 *            ERR_DB_FILE    database file I/O issue
 *            SRCH_NOT_FOUND student was not located in the database
 *
 *  console:  Does not produce any console I/O used by other functions
 */
int get_student(int fd, int id, student_t *s)
{
    student_t temp;     // Temporary student record to hold data read from the file
    ssize_t bytes_read; // Variable to store the number of bytes read from the file

    // Move the file pointer to the beginning of the file (seek offset 0 from the start)
    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        return ERR_DB_FILE; // Return error if seeking the file fails
    }

    // Read the file record by record (each student record has the size of student_t)
    while ((bytes_read = read(fd, &temp, sizeof(student_t))) == sizeof(student_t))
    {
        // Check if the current student's ID matches the requested ID
        if (temp.id == id)
        {
            // If a match is found, copy the student data into the provided student pointer (s)
            memcpy(s, &temp, sizeof(student_t));
            return NO_ERROR; // Return success if the student is found
        }
    }

    // If read fails or doesn't read a full record, return an error
    if (bytes_read == -1)
    {
        return ERR_DB_FILE; // Return error if file reading fails
    }

    // If the student with the requested ID was not found, return a "not found" error
    return SRCH_NOT_FOUND;
}

/*
 *  add_student
 *      fd:     linux file descriptor
 *      id:     student id (range is defined in db.h )
 *      fname:  student first name
 *      lname:  student last name
 *      gpa:    GPA as an integer (range defined in db.h)
 *
 *  Adds a new student to the database.  After calculating the index for the
 *  student, check if there is another student already at that location.  A good
 *  way is to use something like memcmp() to ensure that the location for this
 *  student contains all zero byes indicating the space is empty.
 *
 *  returns:  NO_ERROR       student added to database
 *            ERR_DB_FILE    database file I/O issue
 *            ERR_DB_OP      database operation logically failed (aka student
 *                           already exists)
 *
 *
 *  console:  M_STD_ADDED       on success
 *            M_ERR_DB_ADD_DUP  student already exists
 *            M_ERR_DB_READ     error reading or seeking the database file
 *            M_ERR_DB_WRITE    error writing to db file (adding student)
 *
 */
int add_student(int fd, int id, char *fname, char *lname, int gpa)
{
    student_t student;                              // Declare a student record variable to hold student data
    student_t empty_student = EMPTY_STUDENT_RECORD; // Initialize a placeholder for an empty student record

    // Validate if the ID and GPA are within an acceptable range
    if (validate_range(id, gpa) != NO_ERROR)
    {
        printf("a");      // Print for debugging if validation fails
        return ERR_DB_OP; // Return error if validation fails
    }

    // Calculate the offset in the file based on the student ID
    off_t offset = id * sizeof(student_t);

    // Move the file pointer to the correct position based on the offset
    if (lseek(fd, offset, SEEK_SET) == -1)
    {
        printf("b");        // Print for debugging if seeking the file fails
        return ERR_DB_FILE; // Return error if seeking the file fails
    }

    // Read the student record at the calculated offset into the student variable
    ssize_t bytes_read = read(fd, &student, sizeof(student_t));
    if (bytes_read == -1)
    {
        printf("c");        // Print for debugging if reading from the file fails
        return ERR_DB_FILE; // Return error if reading the file fails
    }

    // Check if a full student record was read and it's not an empty record
    if (bytes_read == sizeof(student_t) && memcmp(&student, &empty_student, sizeof(student_t)) != 0)
    {
        printf("Cant add student with ID=%d, already exists in db.\n", id); // Print error message for existing student
        return ERR_DB_OP;                                                   // Return error for duplicate entry
    }

    // Set the student information (ID, first name, last name, GPA)
    student.id = id;
    strncpy(student.fname, fname, sizeof(student.fname) - 1); // Copy first name (safe copy with max length)
    strncpy(student.lname, lname, sizeof(student.lname) - 1); // Copy last name (safe copy with max length)
    student.gpa = gpa;                                        // Set the GPA

    // Move the file pointer back to the calculated offset for writing
    if (lseek(fd, offset, SEEK_SET) == -1)
    {
        printf("e");        // Print for debugging if seeking the file fails
        return ERR_DB_FILE; // Return error if seeking the file fails
    }

    // Write the updated student record to the file
    if (write(fd, &student, sizeof(student_t)) != sizeof(student_t))
    {
        printf("f");        // Print for debugging if writing to the file fails
        return ERR_DB_FILE; // Return error if writing the file fails
    }

    // Print a success message after the student is added
    printf(M_STD_ADDED, id);
    return NO_ERROR; // Return success
}

/*
 *  del_student
 *      fd:     linux file descriptor
 *      id:     student id to be deleted
 *
 *  Removes a student to the database.  Use the get_student() function to
 *  locate the student to be deleted. If there is a student at that location
 *  write an empty student record - see EMPTY_STUDENT_RECORD from db.h at
 *  that location.
 *
 *  returns:  NO_ERROR       student deleted from database
 *            ERR_DB_FILE    database file I/O issue
 *            ERR_DB_OP      database operation logically failed (aka student
 *                           not in database)
 *
 *
 *  console:  M_STD_DEL_MSG      on success
 *            M_STD_NOT_FND_MSG  student not in database, cant be deleted
 *            M_ERR_DB_READ      error reading or seeking the database file
 *            M_ERR_DB_WRITE     error writing to db file (adding student)
 *
 */
int del_student(int fd, int id)
{
    student_t existing_student;                     // Declare a variable to hold the existing student data
    student_t empty_student = EMPTY_STUDENT_RECORD; // Initialize an empty student record to mark deleted records

    // Use get_student to fetch the student with the given ID from the database
    if (get_student(fd, id, &existing_student) != NO_ERROR)
    {
        // If the student is not found, print an error message and return an error code
        printf(M_STD_NOT_FND_MSG, id); // Display the student not found message
        return ERR_DB_OP;              // Return error if student is not found
    }

    // Now that we have the student, we want to overwrite the record with an empty student record
    off_t offset = id * sizeof(student_t); // Calculate the offset based on the student ID

    // Move the file pointer to the calculated offset for the student record
    if (lseek(fd, offset, SEEK_SET) == -1)
    {
        return ERR_DB_FILE; // Return an error if seeking the file fails
    }

    // Write the empty student record (indicating deletion) to the file at the given offset
    if (write(fd, &empty_student, sizeof(student_t)) != sizeof(student_t))
    {
        return ERR_DB_FILE; // Return an error if writing the file fails
    }

    // Print a success message confirming the student has been deleted
    printf(M_STD_DEL_MSG, id);
    return NO_ERROR; // Return success to indicate the operation was successful
}

/*
 *  count_db_records
 *      fd:     linux file descriptor
 *
 *  Counts the number of records in the database.  Start by reading the
 *  database at the beginning, and continue reading individual records
 *  until you it EOF.  EOF is when the read() syscall returns 0. Check
 *  if a slot is empty or previously deleted by investigating if all of
 *  the bytes in the record read are zeros - I would suggest using memory
 *  compare memcmp() for this. Create a counter variable and initialize it
 *  to zero, every time a non-zero record is read increment the counter.
 *
 *  returns:  <number>       returns the number of records in db on success
 *            ERR_DB_FILE    database file I/O issue
 *            ERR_DB_OP      database operation logically failed (aka student
 *                           not in database)
 *
 *
 *  console:  M_DB_RECORD_CNT  on success, to report the number of students in db
 *            M_DB_EMPTY       on success if the record count in db is zero
 *            M_ERR_DB_READ    error reading or seeking the database file
 *            M_ERR_DB_WRITE   error writing to db file (adding student)
 *
 */
int count_db_records(int fd)
{
    student_t student;                              // Declare a variable to hold the student data read from the file
    student_t empty_student = EMPTY_STUDENT_RECORD; // Initialize an empty student record to compare for deleted records
    int record_count = 0;                           // Initialize a counter for the number of valid records

    off_t offset = 0;   // Start reading from the beginning of the file
    ssize_t bytes_read; // Variable to store the number of bytes read during each read operation

    // Move the file pointer to the beginning of the file
    if (lseek(fd, offset, SEEK_SET) == -1)
    {
        return ERR_DB_FILE; // Return error if seeking the file fails
    }

    // Loop through each student record in the file
    while ((bytes_read = read(fd, &student, sizeof(student_t))) > 0)
    {
        // If the student record is not empty, increment the valid record count
        if (memcmp(&student, &empty_student, sizeof(student_t)) != 0)
        {
            record_count++;
        }
    }

    // If an error occurs while reading the file, return an error
    if (bytes_read == -1)
    {
        return ERR_DB_FILE; // Return error if reading the file fails
    }

    // If no valid records are found, print a message indicating the database is empty
    if (record_count == 0)
    {
        printf(M_DB_EMPTY); // Print message for an empty database
    }
    else
    {
        // Print the total number of valid student records in the database
        printf(M_DB_RECORD_CNT, record_count);
    }

    return record_count; // Return the number of valid records found
}

/*
 *  print_db
 *      fd:     linux file descriptor
 *
 *  Prints all records in the database.  Start by reading the
 *  database at the beginning, and continue reading individual records
 *  until you it EOF.  EOF is when the read() syscall returns 0. Check
 *  if a slot is empty or previously deleted by investigating if all of
 *  the bytes in the record read are zeros - I would suggest using memory
 *  compare memcmp() for this. Be careful as the database might be empty.
 *  on the first real row encountered print the header for the required output:
 *
 *     printf(STUDENT_PRINT_HDR_STRING, "ID",
 *                  "FIRST NAME", "LAST_NAME", "GPA");
 *
 *  then for each valid record encountered print the required output:
 *
 *     printf(STUDENT_PRINT_FMT_STRING, student.id, student.fname,
 *                    student.lname, calculated_gpa_from_student);
 *
 *  The code above assumes you are reading student records into a local
 *  variable named student that is of type student_t. Also dont forget that
 *  the GPA in the student structure is an int, to convert it into a real
 *  gpa divide by 100.0 and store in a float variable.
 *
 *  returns:  NO_ERROR       on success
 *            ERR_DB_FILE    database file I/O issue
 *
 *
 *  console:  <see above>      on success, print table or database empty
 *            M_ERR_DB_READ    error reading or seeking the database file
 *
 */
int print_db(int fd)
{
    student_t student;                              // Declare a variable to hold the student data read from the file
    student_t empty_student = EMPTY_STUDENT_RECORD; // Initialize an empty student record to compare against
    off_t offset = 0;                               // Start reading from the beginning of the file
    ssize_t bytes_read;                             // Variable to store the number of bytes read during each read operation
    int first_valid_record = 1;                     // Flag to track if the first valid record has been printed (to print the header only once)

    // Seek to the beginning of the file
    if (lseek(fd, offset, SEEK_SET) == -1)
    {
        return ERR_DB_FILE; // Return an error if seeking the file fails
    }

    // Read records one by one until EOF
    while ((bytes_read = read(fd, &student, sizeof(student_t))) > 0)
    {
        // Check if the current record is not empty by comparing it with the empty student record
        if (memcmp(&student, &empty_student, sizeof(student_t)) != 0)
        {
            // Print the header only if this is the first valid record
            if (first_valid_record)
            {
                printf(STUDENT_PRINT_HDR_STRING, "ID", "FIRST NAME", "LAST_NAME", "GPA");
                first_valid_record = 0; // Set the flag to false after printing the header
            }

            // Calculate the GPA as a float (dividing by 100 to convert it to a float representation)
            float gpa = student.gpa / 100.0;

            // Print the student information in the formatted output
            printf(STUDENT_PRINT_FMT_STRING, student.id, student.fname, student.lname, gpa);
        }
    }

    // Handle any read errors by checking if bytes_read is -1
    if (bytes_read == -1)
    {
        return ERR_DB_FILE; // Return an error if reading the file fails
    }

    // If no valid records were found (first_valid_record remains true), print a message indicating the database is empty
    if (first_valid_record)
    {
        printf(M_DB_EMPTY); // Print message for an empty database
    }

    return NO_ERROR; // Return success indicating the operation completed without errors
}

/*
 *  print_student
 *      *s:   a pointer to a student_t structure that should
 *            contain a valid student to be printed
 *
 *  Start by ensuring that provided student pointer is valid.  To do this
 *  make sure it is not NULL and that s->id is not zero.  After ensuring
 *  that the student is valid, print it the exact way that is described
 *  in the print_db() function by first printing the header then the
 *  student data:
 *
 *     printf(STUDENT_PRINT_HDR_STRING, "ID",
 *                  "FIRST NAME", "LAST_NAME", "GPA");
 *
 *     printf(STUDENT_PRINT_FMT_STRING, s->id, s->fname,
 *                    student.lname, calculated_gpa_from_s);
 *
 *  Dont forget that  the GPA in the student structure is an int, to convert
 *  it into a real gpa divide by 100.0 and store in a float variable.
 *
 *  returns:  nothing, this is a void function
 *
 *
 *  console:  <see above>      on success, print table or database empty
 *            M_ERR_STD_PRINT  if the function argument s is NULL or if
 *                             s->id is zero
 *
 */
void print_student(student_t *s)
{
    // Check if the pointer is NULL or if the student ID is zero (invalid)
    if (s == NULL || s->id == 0)
    {
        printf(M_ERR_STD_PRINT); // Print the error message
        return;
    }

    // Print the header
    printf(STUDENT_PRINT_HDR_STRING, "ID", "FIRST NAME", "LAST NAME", "GPA");

    // Calculate the GPA as a float
    float gpa = s->gpa / 100.0;

    // Print the student's details
    printf(STUDENT_PRINT_FMT_STRING, s->id, s->fname, s->lname, gpa);
}

/*
 *  NOTE IMPLEMENTING THIS FUNCTION IS EXTRA CREDIT
 *
 *  compress_db
 *      fd:     linux file descriptor
 *
 *  This assignment takes advantage of the way Linux handles sparse files
 *  on disk. Thus if there is a large hole between student records, Linux
 *  will not use any physical storage.  However, when a database record is
 *  deleted storage is used to write a blank - see EMPTY_STUDENT_RECORD from
 *  db.h - record.
 *
 *  Since Linux provides no way to delete data in the middle of a file, and
 *  deleted records take up physical storage, this function will compress the
 *  database by rewriting a new database file that only includes valid student
 *  records. There are a number of ways to do this, but since this is extra credit
 *  you need to figure this out on your own.
 *
 *  At a high level create a temporary database file then copy all valid students from
 *  the active database (passed in via fd) to the temporary file. When this is done
 *  rename the temporary database file to the name of the real database file. See
 *  the constants in db.h for required file names:
 *
 *         #define DB_FILE     "student.db"        //name of database file
 *         #define TMP_DB_FILE ".tmp_student.db"   //for extra credit
 *
 *  Note that you are passed in the fd of the database file to be compressed,
 *  it is very likely you will need to close it to overwrite it with the
 *  compressed version of the file.  To ensure the caller can work with the
 *  compressed file after you create it, it is a good design to return the fd
 *  of the new compressed file from this function
 *
 *  returns:  <number>       returns the fd of the compressed database file
 *            ERR_DB_FILE    database file I/O issue
 *
 *
 *  console:  M_DB_COMPRESSED_OK  on success, the db was successfully compressed.
 *            M_ERR_DB_OPEN    error when opening/creating temporary database file.
 *                             this error should also be returned after you
 *                             compressed the database file and if you are unable
 *                             to open it to pass the fd back to the caller
 *            M_ERR_DB_CREATE  error creating the db file. For instance the
 *                             inability to copy the temporary file back as
 *                             the primary database file.
 *            M_ERR_DB_READ    error reading or seeking the the db or tempdb file
 *            M_ERR_DB_WRITE   error writing to db or tempdb file (adding student)
 *
 */
int compress_db(int fd)
{
    // TODO
    printf(M_NOT_IMPL);
    return fd;
}

/*
 *  validate_range
 *      id:  proposed student id
 *      gpa: proposed gpa
 *
 *  This function validates that the id and gpa are in the allowable ranges
 *  as per the specifications.  It checks if the values are within the
 *  inclusive range using constents in db.h
 *
 *  returns:    NO_ERROR       on success, both ID and GPA are in range
 *              EXIT_FAIL_ARGS if either ID or GPA is out of range
 *
 *  console:  This function does not produce any output
 *
 */
int validate_range(int id, int gpa)
{

    if ((id < MIN_STD_ID) || (id > MAX_STD_ID))
        return EXIT_FAIL_ARGS;

    if ((gpa < MIN_STD_GPA) || (gpa > MAX_STD_GPA))
        return EXIT_FAIL_ARGS;

    return NO_ERROR;
}

/*
 *  usage
 *      exename:  the name of the executable from argv[0]
 *
 *  Prints this programs expected usage
 *
 *  returns:    nothing, this is a void function
 *
 *  console:  This function prints the usage information
 *
 */
void usage(char *exename)
{
    printf("usage: %s -[h|a|c|d|f|p|z] options.  Where:\n", exename);
    printf("\t-h:  prints help\n");
    printf("\t-a id first_name last_name gpa(as 3 digit int):  adds a student\n");
    printf("\t-c:  counts the records in the database\n");
    printf("\t-d id:  deletes a student\n");
    printf("\t-f id:  finds and prints a student in the database\n");
    printf("\t-p:  prints all records in the student database\n");
    printf("\t-x:  compress the database file [EXTRA CREDIT]\n");
    printf("\t-z:  zero db file (remove all records)\n");
}

// Welcome to main()
int main(int argc, char *argv[])
{
    char opt;      // user selected option
    int fd;        // file descriptor of database files
    int rc;        // return code from various operations
    int exit_code; // exit code to shell
    int id;        // userid from argv[2]
    int gpa;       // gpa from argv[5]

    // space for a student structure which we will get back from
    // some of the functions we will be writing such as get_student(),
    // and print_student().
    student_t student = {0};

    // This function must have at least one arg, and the arg must start
    // with a dash
    if ((argc < 2) || (*argv[1] != '-'))
    {
        usage(argv[0]);
        exit(1);
    }

    // The option is the first character after the dash for example
    //-h -a -c -d -f -p -x -z
    opt = (char)*(argv[1] + 1); // get the option flag

    // handle the help flag and then exit normally
    if (opt == 'h')
    {
        usage(argv[0]);
        exit(EXIT_OK);
    }

    // now lets open the file and continue if there is no error
    // note we are not truncating the file using the second
    // parameter
    fd = open_db(DB_FILE, false);
    if (fd < 0)
    {
        exit(EXIT_FAIL_DB);
    }

    // set rc to the return code of the operation to ensure the program
    // use that to determine the proper exit_code.  Look at the header
    // sdbsc.h for expected values.

    exit_code = EXIT_OK;
    switch (opt)
    {
    case 'a':
        //   arv[0] arv[1]  arv[2]      arv[3]    arv[4]  arv[5]
        // prog_name     -a      id  first_name last_name     gpa
        //-------------------------------------------------------
        // example:  prog_name -a 1 John Doe 341
        if (argc != 6)
        {
            usage(argv[0]);
            exit_code = EXIT_FAIL_ARGS;
            break;
        }

        // convert id and gpa to ints from argv.  For this assignment assume
        // they are valid numbers
        id = atoi(argv[2]);
        gpa = atoi(argv[5]);

        exit_code = validate_range(id, gpa);
        if (exit_code == EXIT_FAIL_ARGS)
        {
            printf(M_ERR_STD_RNG);
            break;
        }

        rc = add_student(fd, id, argv[3], argv[4], gpa);
        if (rc < 0)
            exit_code = EXIT_FAIL_DB;

        break;

    case 'c':
        //    arv[0] arv[1]
        // prog_name     -c
        //-----------------
        // example:  prog_name -c
        rc = count_db_records(fd);
        if (rc < 0)
            exit_code = EXIT_FAIL_DB;
        break;

    case 'd':
        //   arv[0]  arv[1]  arv[2]
        // prog_name     -d      id
        //-------------------------
        // example:  prog_name -d 100
        if (argc != 3)
        {
            usage(argv[0]);
            exit_code = EXIT_FAIL_ARGS;
            break;
        }
        id = atoi(argv[2]);
        rc = del_student(fd, id);
        if (rc < 0)
            exit_code = EXIT_FAIL_DB;

        break;

    case 'f':
        //    arv[0] arv[1]  arv[2]
        // prog_name     -f      id
        //-------------------------
        // example:  prog_name -f 100
        if (argc != 3)
        {
            usage(argv[0]);
            exit_code = EXIT_FAIL_ARGS;
            break;
        }
        id = atoi(argv[2]);
        rc = get_student(fd, id, &student);

        switch (rc)
        {
        case NO_ERROR:
            print_student(&student);
            break;
        case SRCH_NOT_FOUND:
            printf(M_STD_NOT_FND_MSG, id);
            exit_code = EXIT_FAIL_DB;
            break;
        default:
            printf(M_ERR_DB_READ);
            exit_code = EXIT_FAIL_DB;
            break;
        }
        break;

    case 'p':
        //    arv[0] arv[1]
        // prog_name     -p
        //-----------------
        // example:  prog_name -p
        rc = print_db(fd);
        if (rc < 0)
            exit_code = EXIT_FAIL_DB;
        break;

    case 'x':
        //    arv[0] arv[1]
        // prog_name     -x
        //-----------------
        // example:  prog_name -x

        // remember compress_db returns a fd of the compressed database.
        // we close it after this switch statement
        fd = compress_db(fd);
        if (fd < 0)
            exit_code = EXIT_FAIL_DB;
        break;

    case 'z':
        //    arv[0] arv[1]
        // prog_name     -x
        //-----------------
        // example:  prog_name -x
        // HINT:  close the db file, we already have fd
        //       and reopen db indicating truncate=true
        close(fd);
        fd = open_db(DB_FILE, true);
        if (fd < 0)
        {
            exit_code = EXIT_FAIL_DB;
            break;
        }
        printf(M_DB_ZERO_OK);
        exit_code = EXIT_OK;
        break;
    default:
        usage(argv[0]);
        exit_code = EXIT_FAIL_ARGS;
    }

    // dont forget to close the file before exiting, and setting the
    // proper exit code - see the header file for expected values
    close(fd);
    exit(exit_code);
}
