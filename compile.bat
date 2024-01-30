@cls
@taskkill /f /im window.exe > nul
@del window.exe > nul
@gcc -g -municode -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -O3 window.c -o window -lgdi32 && window