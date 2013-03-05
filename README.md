CSci 4061 Assignment 3: Piper
===============================================================
Summary
---------------------------------------------------------------
This c program uses system calls related to Process Management  
to execute programs, execute a pipe, wait for these processes to  
finish.

Execution
---------------------------------------------------------------
\> ./pipe

input: ls -l | grep ^d | wc -l

The program will then tokenize the string sperating by "|" and  
execute the commands in a piped fashion.

It will also store the information in a LOGFILE
