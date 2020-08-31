smallsh.c: a C program to function similar to a bash terminal.
testScript.sh: a bash program to test out the functions included in smallsh.c
How to Compile:
gcc -o smallsh smallsh.c -std=c99
chmod +x testScript.sh

How to Run:
./smallsh

How to Run with Grading Script:
testScript > myTestResults 2>&1
