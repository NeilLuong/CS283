1. In this assignment I suggested you use `fgets()` to get user input in the main while loop. Why is `fgets()` a good choice for this application?

    > **Answer**:  The libc fgets() function is a good choice here because it reads input as "lines of input," which makes sure that the program can handle user input safely and predictably.

2. You needed to use `malloc()` to allocte memory for `cmd_buff` in `dsh_cli.c`. Can you explain why you needed to do that, instead of allocating a fixed-size array?

    > **Answer**:  Using malloc() for cmd_buff provides flexibility compared to a fixed-size array. A fixed array has a compile-time size, which may waste memory if oversized or risk overflow if too small. Dynamic allocation ensures efficient memory use, allows runtime adjustments, and prevents large buffers from consuming limited stack space, improving scalability and adaptability.


3. In `dshlib.c`, the function `build_cmd_list(`)` must trim leading and trailing spaces from each command before storing it. Why is this necessary? If we didn't trim spaces, what kind of issues might arise when executing commands in our shell?

    > **Answer**:  Trimming spaces in build_cmd_list() ensures accurate command parsing and execution. Without trimming, commands like " ls " may be misinterpreted, leading to execution errors or incorrect argument passing. It prevents empty commands, ensures proper matching (e.g., "exit" vs. " exit "), and avoids unexpected behavior in the shell.

4. For this question you need to do some research on STDIN, STDOUT, and STDERR in Linux. We've learned this week that shells are "robust brokers of input and output". Google _"linux shell stdin stdout stderr explained"_ to get started.

- One topic you should have found information on is "redirection". Please provide at least 3 redirection examples that we should implement in our custom shell, and explain what challenges we might have implementing them.

> **Answer**:  

- **Output Redirection (`>` and `>>`)** – Redirects `STDOUT` to a file (`ls > output.txt`),  
  overwriting (`>`) or appending (`>>`).  
  - **Challenge**: Handling file permissions and ensuring proper file creation or appending.  

- **Input Redirection (`<`)** – Reads input from a file instead of `STDIN` (`sort < input.txt`).  
  - **Challenge**: Ensuring the file exists and handling errors when reading.  

- **Error Redirection (`2>`)** – Redirects `STDERR` to a file (`gcc program.c 2> errors.txt`).  
  - **Challenge**: Managing separate output/error streams and avoiding accidental overwrites.  


- You should have also learned about "pipes". Redirection and piping both involve controlling input and output in the shell, but they serve different purposes. Explain the key differences between redirection and piping.

    > **Answer**:  Redirection controls where a command reads from or writes to, typically involving files (e.g., `ls > output.txt`). Piping (`|`), however, connects the output of one command as the input to another (e.g., `ls | grep txt`), enabling efficient data processing. Pipes are temporary and used for inter-process communication, whereas redirection stores data for later use.


- STDERR is often used for error messages, while STDOUT is for regular output. Why is it important to keep these separate in a shell?

    > **Answer**:  Separating STDOUT and STDERR prevents error messages from interfering with expected output. This distinction allows scripts and programs to process results independently while still capturing errors for debugging. Merging them by default could cause unintended data corruption or misinterpretation.

- How should our custom shell handle errors from commands that fail? Consider cases where a command outputs both STDOUT and STDERR. Should we provide a way to merge them, and if so, how?

    > **Answer**:  Our shell should display `STDERR` separately by default but provide an option to merge it with `STDOUT` using `2>&1` (e.g., `command > output.txt 2>&1`). Handling failures should involve checking exit codes and printing meaningful error messages. If both `STDOUT` and `STDERR` are redirected, the shell should ensure proper ordering to avoid jumbled output.
