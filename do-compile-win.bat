@rem windows compile of swiss file knife using msvc 14.
@rem compile under MinGW/Cygwin is also possible, see below.
@rem configure/make will not work under Windows as it is now.
@echo off

IF "%1"=="mingw" GOTO mingw
IF "%1"=="trace" GOTO xtrace

cl /Fesfk.exe -Isrc src\sfk.cpp src\sfkext.cpp src\sfkpack.cpp kernel32.lib user32.lib gdi32.lib ws2_32.lib advapi32.lib shell32.lib
set MTK_TRACE=
GOTO xdone

:xtrace
echo "compiling trace version"
cl /Fesfk.exe -DWITH_TRACING -DVERBOSE_MEM -Isrc src\sfk.cpp src\sfkext.cpp src\sfkpack.cpp kernel32.lib user32.lib gdi32.lib ws2_32.lib advapi32.lib shell32.lib
set MTK_TRACE=file:twexb,filename:log.txt
GOTO xdone

:mingw
rem Example for compile with MinGW/Cygwin:
g++ -osfk.exe -Isrc src\sfk.cpp src\sfkext.cpp src\sfkpack.cpp -lkernel32 -luser32 -lgdi32 -lws2_32 -ladvapi32 -lshell32

:xdone
