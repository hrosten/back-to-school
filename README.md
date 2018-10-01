# Solution to wunderpahkina-vol9  
Wunderdog's **Back to school** solution in C (and Python). 
For more details see: [wunderpahkina-vol9](https://github.com/wunderdogsw/wunderpahkina-vol9).

## Getting started
You need a C compiler and related tools to compile the program.

Using GNU Compiler Collection as an example:
```
foo@bar:~$ gcc back-to-school.c -o back-to-school
```

Then, run the program:
```
foo@bar:~$ ./back-to-school <input_file_name_here>
```

## Other notes:
Feel free to change the value of the preprocessor define MAX_LINE_LEN if the program needs to handle input lines longer than 10240 characters. See line 11 in back-to-school.c:

```
#define MAX_LINE_LEN 1024*10
```
