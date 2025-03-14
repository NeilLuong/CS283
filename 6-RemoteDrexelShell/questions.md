1. How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?

> **Answer**: A special EOF character (RDSH_EOF_CHAR) sent via send_message_eof() after command execution completes used to determine. So the client knows to stop reading when it receives this marker.  As shown in your exec_client_requests(), the client accumulates data chunks until a complete message is formed, using dynamically growing buffers to handle messages of unknown size. The code demonstrates reading chunks of data in a loop, processing and accumulating them until the termination condition is met.

2. This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?

> **Answer**: Shell protocol uses delimiter-based framing - null characters (\0) mark the end of commands sent from client to server, and a special EOF character marks command output completion. Several problems might occur without proper boundary handling: 1) Message confusion: Commands might merge or fragment, 2) Protocol desynchronization: Once framing is lost, it's difficult to recover proper alignment, 3) Deadlocks: Clients may wait indefinitely for missing end markers, 4) Buffer overflows: Without size limits, receivers risk memory corruption.

3. Describe the general differences between stateful and stateless protocols.

> **Answer**: Stateful Protocols: Maintain client session information across requests, remember previous interactions for context in current operations, require connection establishment and teardown phases, examples: FTP, SSH, and your remote shell protocol, more efficient for related operations but need resources for state, vulnerable to disruptions (server crashes lose state)

> **Answer**: Stateless Protocols: Treat each request independently with no relation to previous ones, don't maintain client state between requests, each request contains all information needed to complete it, examples: HTTP (basic form), DNS More resilient to failures but may require more bandwidth, easier to scale horizontally

4. Our lecture this week stated that UDP is "unreliable". If that is the case, why would we ever use it?

> **Answer**: It has lower latency, simpler implementations, and reduced overhead. TCP was appropriate since command execution requires reliable, ordered delivery.

5. What interface/abstraction is provided by the operating system to enable applications to use network communications?

> **Answer**: The socket interface (socket API) serves as the primary operating system abstraction for network communications.