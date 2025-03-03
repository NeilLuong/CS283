1. Your shell forks multiple child processes when executing piped commands. How does your implementation ensure that all child processes complete before the shell continues accepting user input? What would happen if you forgot to call waitpid() on all child processes?

> **Answer**: My implementation ensures all child processes complete before the shell continues accepting user input by: Storing all child process PIDs in an array (child_pids), using a loop to call waitpid() on each PID after creating all processes, only returning from execute_pipeline() after all children have been waited for.
If we forgot to call waitpid() on all child processes: zombie processes would accumulate in the system, taking up process table entries. The shell would not properly capture the exit status of commands, the parent shell would be unable to detect failures in child processes and resource leaks would occur.

2. The dup2() function is used to redirect input and output file descriptors. Explain why it is necessary to close unused pipe ends after calling dup2(). What could go wrong if you leave pipes open?

> **Answer**: Because every open file descriptor consumes kernel resources and open pipes prevent processes from receiving EOF signals properly
What could go wrong are endless waiting, resource leaks, data starvations and deadlocks

3. Your shell recognizes built-in commands (cd, exit, dragon). Unlike external commands, built-in commands do not require execvp(). Why is cd implemented as a built-in rather than an external command? What challenges would arise if cd were implemented as an external process?

> **Answer**: If cd were an external command, it would change the directory only for its own process, and when it exited, the parent shell would remain in the original directory

4. Currently, your shell supports a fixed number of piped commands (CMD_MAX). How would you modify your implementation to allow an arbitrary number of piped commands while still handling memory allocation efficiently? What trade-offs would you need to consider?

> **Answer**: Replace fixed-size pipes with a dynamically allocated array. Replace the fixed-size command list with a linked list or dynamically resized array. Process commands in chunks if they exceed a certain size.
Trade-offs: Dynamic allocation allows arbitrary length but requires more complex memory management, more allocations mean more error conditions to handle, linked lists provide flexibility but with poorer cache locality, a fixed limit is simpler to implement and debug, and even with dynamic allocation, the system will still have limits on total pipes and processes
