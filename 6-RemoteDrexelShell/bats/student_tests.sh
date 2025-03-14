#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file

@test "Example: check ls runs without errors" {
    run ./dsh <<EOF                
ls
EOF

    # Assertions
    [ "$status" -eq 0 ]
}

@test "Basic command execution" {
    run ./dsh <<EOF
echo "hello world"
exit
EOF

    echo "Output: $output"
    [[ "$output" == *"hello world"* ]]
    [ "$status" -eq 0 ]
}

@test "Simple pipe" {
    run ./dsh <<EOF
echo "hello world" | grep "hello"
exit
EOF

    echo "Output: $output"
    [[ "$output" == *"hello world"* ]]
    [ "$status" -eq 0 ]
}

@test "Multiple pipes" {
    run ./dsh <<EOF
echo -e "line1\nline2\nline3" | grep "line" | wc -l
exit
EOF

    echo "Output: $output"
    [[ "$output" == *"3"* ]]
    [ "$status" -eq 0 ]
}

@test "Built-in cd command" {
    run ./dsh <<EOF
cd /tmp
pwd
exit
EOF

    echo "Output: $output"
    [[ "$output" == *"/tmp"* ]]
    [ "$status" -eq 0 ]
}

@test "Command not found" {
    run ./dsh <<EOF
nonexistentcommand12345
exit
EOF

    echo "Output: $output"
    [[ "$output" == *"Command not found"* ]]
    [ "$status" -eq 0 ]  # Shell should continue after command not found
}

@test "Server startup and shutdown" {
    # Find an available port
    TEST_PORT=$(shuf -i 20000-30000 -n 1)
    
    # Start server in background with the available port
    ./dsh -s -p $TEST_PORT > server_output.tmp 2>&1 &
    SERVER_PID=$!
    
    # Give server time to start
    sleep 3
    
    # Verify server is running
    if ! ps -p $SERVER_PID > /dev/null; then
        echo "ERROR: Server failed to start"
        cat server_output.tmp
        return 1
    fi
    
    echo "Server started with PID $SERVER_PID"
    
    # Try graceful shutdown first
    echo "Attempting graceful shutdown with SIGTERM"
    kill $SERVER_PID
    sleep 2
    
    # If still running, force kill
    if ps -p $SERVER_PID > /dev/null; then
        echo "Server didn't respond to SIGTERM, using SIGKILL"
        kill -9 $SERVER_PID
        sleep 2
    fi
    
    # Verify server is stopped
    if ps -p $SERVER_PID > /dev/null; then
        echo "ERROR: Server failed to shut down even with SIGKILL"
        ps -f $SERVER_PID
        return 1
    else
        echo "Server successfully shut down"
    fi
    
    # Clean up
    rm -f server_output.tmp
}

@test "Client tests with server running" {
    # Start server in background
    TEST_PORT=25123
    ./dsh -s -p $TEST_PORT &
    SERVER_PID=$!
    sleep 2  # Give server time to start
    
    # Run client tests
    echo "exit" | ./dsh -c "localhost:$TEST_PORT"
    
    # Verify client connected and exited successfully
    [ "$?" -eq 0 ]
    
    # Clean up server
    kill $SERVER_PID
    sleep 1
}

@test "Client connection to server" {
    # Start server with a fixed port (easier to debug)
    TEST_PORT=25884
    
    # Kill any process using the port (just to be safe)
    pkill -f "dsh -s -p $TEST_PORT" || true
    sleep 1
    
    # Start server in background with explicit port
    ./dsh -s -p $TEST_PORT > server_output.log 2>&1 &
    SERVER_PID=$!
    
    # Give server time to start
    sleep 3
    
    # Debug: Check if server is running and on which port
    ps -f -p $SERVER_PID
    netstat -tuln | grep $TEST_PORT || echo "No process listening on port $TEST_PORT"
    
    # Now for the client - try different formats to connect
    
    # Format 1: Try with space between -c and address
    echo "echo 'Testing connection'" | ./dsh -c 127.0.0.1:$TEST_PORT > client_output.log 2>&1
    cat client_output.log
    
    # Format 2: Try with address and port separately
    echo "echo 'Testing connection'" | ./dsh -c 127.0.0.1 -p $TEST_PORT > client_output2.log 2>&1
    cat client_output2.log
    
    # Format 3: Try with full address:port in quotes
    echo "echo 'Testing connection'" | ./dsh -c "127.0.0.1:$TEST_PORT" > client_output3.log 2>&1
    cat client_output3.log
    
    # Grab one that worked, if any
    if grep -q "Testing connection" client_output.log; then
        output=$(cat client_output.log)
    elif grep -q "Testing connection" client_output2.log; then
        output=$(cat client_output2.log)
    elif grep -q "Testing connection" client_output3.log; then
        output=$(cat client_output3.log)
    else
        output="None of the client connection attempts succeeded"
    fi
    
    # Output for debugging
    echo "Final output: $output"
    
    # Clean up
    kill $SERVER_PID || true
    sleep 2
    rm -f server_output.log client_output*.log
    
    # Check results (output contains success message)
    [[ "$output" == *"Testing connection"* ]]
}

@test "Remote: Simple command" {
    
    # Start server in background with a unique port
    TEST_PORT=12346
    ./dsh -s -p $TEST_PORT &
    SERVER_PID=$!
    
    # Give server time to start
    sleep 1
    
    # Run client command
    run ./dsh -c -p $TEST_PORT <<EOF
echo "hello from remote"
exit
EOF

    # Stop the server
    kill $SERVER_PID
    
    echo "Output: $output"
    [[ "$output" == *"hello from remote"* ]]
    [ "$status" -eq 0 ]
}

@test "Remote: Pipes" {
    
    # Start server in background with a unique port
    TEST_PORT=12347
    ./dsh -s -p $TEST_PORT &
    SERVER_PID=$!
    
    # Give server time to start
    sleep 1
    
    # Run client command with pipes
    run ./dsh -c -p $TEST_PORT <<EOF
ls | grep dshlib.c
exit
EOF

    # Stop the server
    kill $SERVER_PID
    
    echo "Output: $output"
    [[ "$output" == *"dshlib.c"* ]]
    [ "$status" -eq 0 ]
}

@test "Remote: Built-in command cd" {
    
    # Start server in background with a unique port
    TEST_PORT=12348
    ./dsh -s -p $TEST_PORT &
    SERVER_PID=$!
    
    # Give server time to start
    sleep 1
    
    # Run cd command and then check pwd
    run ./dsh -c -p $TEST_PORT <<EOF
cd /tmp
pwd
exit
EOF

    # Stop the server
    kill $SERVER_PID
    
    echo "Output: $output"
    [[ "$output" == *"/tmp"* ]]
    [ "$status" -eq 0 ]
}

@test "Remote: stop-server command" {
    # Start server in background with a unique port
    TEST_PORT=12349
    ./dsh -s -p $TEST_PORT &
    SERVER_PID=$!
    
    # Give server time to start
    sleep 2
    
    # Verify server is running before sending command
    ps -p $SERVER_PID > /dev/null || {
        echo "Server failed to start properly"
        return 1
    }
    
    # Run stop-server command
    echo "stop-server" | ./dsh -c localhost -p $TEST_PORT
    
    # Give more time for server to shut down
    sleep 3
    
    # Check if server process still exists
    if ps -p $SERVER_PID > /dev/null; then
        echo "ERROR: Server process $SERVER_PID still running!"
        kill -9 $SERVER_PID  # Force kill it to clean up
        return 1
    else
        echo "Server successfully terminated"
    fi
}

# Network error handling tests

@test "Connection to non-existent server" {
    # Try to connect to a port where no server is running
    OUTPUT=$(./dsh -c localhost -p 65432 2>&1 || true)
    STATUS_CODE=$?
    
    echo "Output: $OUTPUT"
    echo "Status code: $STATUS_CODE"
    
    # Check for connection failure message
    [[ "$OUTPUT" == *"connection failed"* ]]
}

@test "Shell starts and accepts input" {
    run "./dsh" <<EOF
echo "Hello, dsh!"
EOF

    [ "$status" -eq 0 ]
    [[ "$output" =~ "Hello, dsh!" ]]
}

@test "exit command" {
    run "./dsh" <<EOF
exit
EOF

    [ "$status" -eq 0 ]
}

@test "Pipe with multiple commands" {
    run ./dsh <<EOF
printf "line1\nline2\nline3\n" | grep line | wc -l
EOF

    echo "Command output: $output"
    [ "$status" -eq 0 ]
    [[ "$output" == *"3"* ]]
}

@test "Multiple pipes in command" {
    run ./dsh <<EOF
echo "one two three four" | tr ' ' '\n' | sort | grep o
EOF

    [ "$status" -eq 0 ]
    [[ "$output" == *"four"* ]]
    [[ "$output" == *"one"* ]]
    [[ "$output" != *"three"* ]]
}

@test "Command with pipe followed by regular command" {
    run ./dsh <<EOF
echo hello | grep hello
echo another command
EOF

    [ "$status" -eq 0 ]
    [[ "$output" == *"hello"* ]]
    [[ "$output" == *"another command"* ]]
}
