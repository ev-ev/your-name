@cls
@taskkill /f /im window.exe > nul
@del window.exe > nul
@gcc -g -O3 -municode -pthread -D DBGM -Wall -Wextra -Woverflow -Wno-unused-parameter -Wno-sign-compare text/window.c -o window -lgdi32 -lole32 -luuid -lcomdlg32 -lshcore -lWinmm && window