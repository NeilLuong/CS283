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

#!/usr/bin/env bats

# File: student_tests.sh
# 
# Test suite for shell implementation with pipe support

# Basic command execution
@test "Check ls runs without errors" {
    run ./dsh <<EOF                
ls
EOF

    [ "$status" -eq 0 ]
}

@test "Check echo works" {
    run ./dsh <<EOF
echo hello world
EOF

    [ "$status" -eq 0 ]
    [[ "$output" == *"hello world"* ]]
}

@test "Check multiple commands run successfully" {
    run ./dsh <<EOF
echo first command
echo second command
EOF

    [ "$status" -eq 0 ]
    [[ "$output" == *"first command"* ]]
    [[ "$output" == *"second command"* ]]
}

@test "Check command with arguments" {
    run ./dsh <<EOF
echo one two three four
EOF

    [ "$status" -eq 0 ]
    [[ "$output" == *"one two three four"* ]]
}

# Checking built-in commands
@test "Check cd command changes directory" {
    run ./dsh <<EOF
mkdir -p /tmp/dsh_test_dir
cd /tmp/dsh_test_dir
pwd
EOF

    [ "$status" -eq 0 ]
    [[ "$output" == *"/tmp/dsh_test_dir"* ]]
    rmdir /tmp/dsh_test_dir
}

@test "Check 'rc' command returns last exit code" {
    run ./dsh <<EOF
true
rc
false
rc
EOF

    [ "$status" -eq 0 ]
    [[ "$output" == *"0"* ]]
    [[ "$output" == *"1"* ]]
}

@test "Check exit command" {
    run ./dsh <<EOF
exit
echo this should not run
EOF

    [ "$status" -eq 0 ]
    [[ "$output" != *"this should not run"* ]]
}

# Testing pipe functionality
@test "Basic pipe operation" {
    run ./dsh <<EOF
echo hello world | grep hello
EOF

    [ "$status" -eq 0 ]
    [[ "$output" == *"hello world"* ]]
}

@test "Pipe with multiple commands" {
    run ./dsh <<EOF
echo "line1\nline2\nline3" | grep line | wc -l
EOF

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

@test "Redirect command output to file then read it" {
    run ./dsh <<EOF
echo test output > /tmp/dsh_test_file
cat /tmp/dsh_test_file
EOF

    [ "$status" -eq 0 ]
    [[ "$output" == *"test output"* ]]
    rm -f /tmp/dsh_test_file
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

# Edge cases and error handling
@test "Error: command not found" {
    run ./dsh <<EOF
nonexistentcommand
EOF

    [[ "$output" == *"not found"* ]]
}

@test "Error: handle empty command" {
    run ./dsh <<EOF

EOF

    [ "$status" -eq 0 ]
    [[ "$output" == *"warning: no commands provided"* ]]
}

@test "Error: handle whitespace only command" {
    run ./dsh <<EOF
   
EOF

    [ "$status" -eq 0 ]
    [[ "$output" == *"warning: no commands provided"* ]]
}

@test "Error: handle permission denied" {
    run ./dsh <<EOF
echo "#!/bin/sh\necho test" > /tmp/dsh_test_script
chmod 644 /tmp/dsh_test_script
/tmp/dsh_test_script
EOF

    [[ "$output" == *"Permission denied"* ]]
    rm -f /tmp/dsh_test_script
}

# Complex test cases
@test "Combine pipes with file operations" {
    run ./dsh <<EOF
echo "apple\nbanana\ncherry\ndragonfruit" > /tmp/dsh_fruits
cat /tmp/dsh_fruits | grep a | sort -r
EOF

    [ "$status" -eq 0 ]
    [[ "$output" == *"dragonfruit"* ]]
    [[ "$output" == *"banana"* ]]
    [[ "$output" == *"apple"* ]]
    # cherry doesn't have 'a'
    [[ "$output" != *"cherry"* ]]
    rm -f /tmp/dsh_fruits
}

@test "Pipe between multiple grep commands" {
    run ./dsh <<EOF
echo "apple\nbanana\ncherry\ndragonfruit\napricot" > /tmp/dsh_pipe_test
cat /tmp/dsh_pipe_test | grep a | grep r | sort
EOF

    [ "$status" -eq 0 ]
    [[ "$output" == *"apricot"* ]]
    [[ "$output" == *"dragonfruit"* ]]
    # banana has 'a' but no 'r'
    [[ "$output" != *"banana"* ]]
    # apple has 'a' but no 'r'
    [[ "$output" != *"apple"* ]]
    # cherry has 'r' but no 'a'
    [[ "$output" != *"cherry"* ]]
    rm -f /tmp/dsh_pipe_test
}

@test "Create file, use in pipeline, count lines" {
    run ./dsh <<EOF
echo -e "line1\nline2\nline3\nline4\nline5" > /tmp/dsh_lines
cat /tmp/dsh_lines | grep line | wc -l
EOF

    [ "$status" -eq 0 ]
    [[ "$output" == *"5"* ]]
    rm -f /tmp/dsh_lines
}

@test "Pipe with number counting" {
    run ./dsh <<EOF
echo "1 2 3 4 5 6 7 8 9 10" | tr ' ' '\n' | grep -v "1" | wc -l
EOF

    [ "$status" -eq 0 ]
    # Should only count 2,3,4,5,6,7,8,9 (8 lines)
    # 1 and 10 are filtered out by grep -v "1"
    [[ "$output" == *"8"* ]]
}

# Testing resource cleanup
@test "Check no file descriptor leaks with multiple pipes" {
    run ./dsh <<EOF
ls | grep a | grep b | grep c | grep d | grep e
# Run another complex command to verify no descriptor leaks
find /tmp -type f -name "dsh_*" | wc -l
EOF

    [ "$status" -eq 0 ]
}

@test "Rapidly create and close many pipes" {
    run ./dsh <<EOF
for i in {1..5}; do
  echo "Test $i" | cat | sort | uniq | wc -l
done
EOF

    [ "$status" -eq 0 ]
    [[ "$output" == *"1"* ]]
}

# Performance/stress tests
@test "Run multiple commands with pipes in sequence" {
    run ./dsh <<EOF
echo "test1" | cat | grep test
echo "test2" | cat | grep test
echo "test3" | cat | grep test
echo "test4" | cat | grep test
echo "test5" | cat | grep test
EOF

    [ "$status" -eq 0 ]
    [[ "$output" == *"test1"* ]]
    [[ "$output" == *"test2"* ]]
    [[ "$output" == *"test3"* ]]
    [[ "$output" == *"test4"* ]]
    [[ "$output" == *"test5"* ]]
}

@test "Process large data through pipes" {
    run ./dsh <<EOF
seq 1 1000 | grep -v "5" | wc -l
EOF

    [ "$status" -eq 0 ]
    # 1000 numbers, removing all numbers containing 5 (like 5, 15, 25, etc.)
    # In 1-1000, there are 271 numbers containing digit 5
    [[ "$output" == *"729"* ]]
}

@test "Deep pipeline with 5 commands" {
    run ./dsh <<EOF
echo "a b c d e f g h i j" | tr ' ' '\n' | sort -r | head -n 5 | tr '\n' ','
EOF

    [ "$status" -eq 0 ]
    [[ "$output" == *"j,i,h,g,f,"* ]]
}
