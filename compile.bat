@cls
@taskkill /f /im window.exe > nul
@del window.exe > nul
@del yourname.exe > nul
@gcc -g -municode -D DBGM -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare text/window.c -o window -lgdi32 -lole32 -luuid -lcomdlg32 && window
rem gcc -O3 -mwindows -municode -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare text/window.c -o window -lgdi32 -lole32 -luuid -lcomdlg32