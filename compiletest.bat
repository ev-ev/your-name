@cls
@taskkill /f /im test.exe> nul
@del test.exe> nul
@gcc -g -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -O3 test/test.c -o test  && test