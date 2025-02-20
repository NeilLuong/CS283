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

@test "Built-in exit command terminates the shell" {
  run ./dsh <<EOF
exit
EOF
  [ "$status" -eq 0 ]
}

@test "Built-in cd command with no argument does not change directory" {
  initial_pwd=$(pwd)
  run ./dsh <<EOF
cd
pwd
exit
EOF
  echo "$output" | grep -q "$initial_pwd"
  [ "$status" -eq 0 ]
}

@test "Built-in cd command with argument changes directory" {
  run ./dsh <<EOF
cd /tmp
pwd
exit
EOF
  echo "$output" | grep -q "/tmp"
  [ "$status" -eq 0 ]
}

@test "Built-in dragon command prints the dragon" {
  run ./dsh <<EOF
dragon
exit
EOF
  echo "$output" | grep -q "@%%%%"
  [ "$status" -eq 0 ]
}

@test "Empty input prints warning" {
  run ./dsh <<EOF
       
exit
EOF
  echo "$output" | grep -q "warning: no commands provided"
  [ "$status" -eq 0 ]
}

@test "Spaces-only input prints warning" {
  run ./dsh <<EOF
      
exit
EOF
  echo "$output" | grep -q "warning: no commands provided"
  [ "$status" -eq 0 ]
}

@test "cd with extra arguments uses only first argument" {
  run ./dsh <<EOF
cd /tmp /var
pwd
exit
EOF
  echo "$output" | grep -q "/tmp"
  [ "$status" -eq 0 ]
}

@test "Echo with multiple spaces unquoted" {
  run ./dsh <<EOF
echo    hello      world
exit
EOF
  echo "$output" | grep -q "hello world"
  [ "$status" -eq 0 ]
}

@test "Mixed quoted and unquoted tokens in echo" {
  run ./dsh <<EOF
echo foo "bar baz" qux
exit
EOF
  echo "$output" | grep -q "foo bar baz qux"
  [ "$status" -eq 0 ]
}

@test "uname -a returns kernel info" {
  run ./dsh <<EOF
uname -a
exit
EOF
  echo "$output" | grep -q "^Darwin"
  [ "$status" -eq 0 ]
}

@test "uname returns Darwin" {
  run ./dsh <<EOF
uname
exit
EOF
  echo "$output" | grep -q "^Darwin$"
  [ "$status" -eq 0 ]
}

@test "echo prints 'hello,      world'" {
  run ./dsh <<EOF
echo "hello,      world"
exit
EOF
  echo "$output" | grep -q "hello,      world"
  [ "$status" -eq 0 ]
}

@test "pwd returns the current working directory" {
  current=$(pwd)
  run ./dsh <<EOF
pwd
exit
EOF
  echo "$output" | grep -q "$current"
  [ "$status" -eq 0 ]
}

@test "ls lists expected files" {
  run ./dsh <<EOF
ls
exit
EOF
  echo "$output" | grep -q "dshlib.h"
  echo "$output" | grep -q "makefile"
  [ "$status" -eq 0 ]
}

@test "cd changes directory and pwd reflects the change" {
  run ./dsh <<EOF
cd ..
pwd
exit
EOF
  echo "$output" | grep -q "CS283"
  [ "$status" -eq 0 ]
}

@test "rc builtin prints 0 after a successful command" {
  run ./dsh <<EOF
uname
rc
exit
EOF
  result_line=$(echo "$output" | sed '$d' | tail -n 1 | awk '{print $NF}')
  [ "$result_line" = "0" ]
  [ "$status" -eq 0 ]
}

@test "rc builtin prints error code after a failing command" {
  run ./dsh <<EOF
non_exists
rc
exit
EOF
  last_line=$(echo "$output" | sed '$d' | tail -n 1 | awk '{print $NF}')
  
  [ "$last_line" = "2" ]
  [ "$status" -eq 0 ]
}


