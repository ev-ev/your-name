@cls
@taskkill /f /im yourname.exe > nul
@del yourname.exe > nul
gcc -O3 -mwindows -municode -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare text/window.c -o yourname -lgdi32 -lole32 -luuid -lcomdlg32 -lshcore