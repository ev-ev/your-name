@cls
@taskkill /f /im window.exe > nul
@del window.exe > nul
@del yourname.exe > nul
@gcc -g -municode -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -O3 text/window.c -o window -lgdi32 -lole32 -luuid && window