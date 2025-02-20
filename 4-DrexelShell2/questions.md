1. Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?

    > **Answer**:  Using fork() creates a child process and that allows both the parent and the child to run processes in parallel. This mechanism allows the parent to avoid itself from terminating as opposed to directly calling execvp
    

2. What happens if the fork() system call fails? How does your implementation handle this scenario?

    > **Answer**:  It will return a -1 if fork() fails and set the variable errno to indicate which error it is which includes EAGAIN (means that resource has reached a limit), ENOMEM (insufficient memory) or a default error message

3. How does execvp() find the command to execute? What system environment variable plays a role in this process?

    > **Answer**:  The execvp() function searches for the command to execute by scanning the directories listed in the PATH environment variable.

4. What is the purpose of calling wait() in the parent process after forking? What would happen if we didn’t call it?

    > **Answer**:  Calling wait() in the parent process makes sure that the parent suspends its execution until the child process has finished to prevent the creation of zombie processes. Without no wait, uncontrollable bahaviors might happen

5. In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?

    > **Answer**:  WEXITSTATUS() extracts the exit code from the status value returned by wait to know if the child is terminated correctly which makes it important to the parent because then it will know if the command is executed correctly or not

6. Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?

    > **Answer**:  The implementation of build_cmd_buff() handles quoted arguments by first checking if a token begins with a single (') or double (") quote. When a quote is detected, the function skips the opening quote and then reads characters continuously until it finds the matching closing quote, at which point it replaces the closing quote with a null terminator. This makes sures that all characters between the quotes—including any spaces—are treated as a single token. Handling quoted arguments in this way is necessary because many command-line inputs include arguments with spaces (such as file names or phrases) that need to be processed as one cohesive unit rather than being split into multiple tokens.

7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?

    > **Answer**:  Handling quoted args was new. Aside from that and the added functionality and the fact that its a general command line parser now, nothing has changed much in terms of logic

8. For this quesiton, you need to do some research on Linux signals. You can use [this google search](https://www.google.com/search?q=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&oq=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzc2MGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8) to get started.

- What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?

    > **Answer**:  Signals are interrupts in software for interprocess communication. What differs them from IPC methods are: they are asynchronous notifications sent to processes to trigger specific behaviors, they are more lightweight than other IPC methods, they are primarily used for process control and error handling rather than data transfer

- Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?

    > **Answer**:  SIGKILL is immediate process termination and cannot be caught or ignored and is typically used for force killing unresponsive processes. SIGTERM is the default termination signal which allows processes to catch the signal and perform cleanup before exiting. SIGTERM is typically used for normal processes shutdonw. SIGINT is interruption from keyboard that can be caught and handled and is typically used for user-initiated termination

- What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?

    > **Answer**:  The process is immediately paused and it cannot be caught or ignored because by design, this is done to ensure that the OS or a person can reliably pause a process
