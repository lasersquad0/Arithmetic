@echo off

rem Здесь тестируем все алгоритмы на правильность работы.
rem Тестируем на одном файле.
rem Проверяем совпадение файлов после сжатия-расжатия

@echo --------------compressing -----------------
@set FILE=primes - 0B-1M.simple.txt
@set FOLDER=tmp
@set OPT=-b 1M -t 1

@rem for O0FIX model we use OPT2 instead of OPT, see below
@set OPT2=-s -t 1


@copy /Y ..\x64\Release\ArithmeticConsole.exe .
@rem @copy /Y ..\x64\Release\ArithmeticLog.lfg .

@set FILEOUT=%FOLDER%\%FILE%.o1.ari
@copy "%FILE%" "%FILEOUT%"
@ArithmeticConsole.exe %OPT% -m o1 -c ari -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.o1.ari32
@copy "%FILE%" "%FILEOUT%"
@ArithmeticConsole.exe %OPT% -m o1 -c ari32 -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.o1.ari64
@copy "%FILE%" "%FILEOUT%"
@ArithmeticConsole.exe %OPT% -m o1 -c ari64 -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.o1.bitari
@copy "%FILE%" "%FILEOUT%"
@ArithmeticConsole.exe %OPT% -m o1 -c bitari -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.o2.ari
@copy "%FILE%" "%FILEOUT%"
@ArithmeticConsole.exe %OPT% -m o2 -c ari -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.o2.ari32
@copy "%FILE%" "%FILEOUT%"
@ArithmeticConsole.exe %OPT% -m o2 -c ari32 -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.o2.ari64
@copy "%FILE%" "%FILEOUT%"
@ArithmeticConsole.exe %OPT% -m o2 -c ari64 -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.o2.bitari
@copy "%FILE%" "%FILEOUT%"
@ArithmeticConsole.exe %OPT% -m o2 -c bitari -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O0FIX.ari
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT2% -m O0FIX -c ari -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O0FIX.ari32
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT2% -m O0FIX -c ari32 -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O0FIX.ari64
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT2% -m O0FIX -c ari64 -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O0FIX.bitari
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT2% -m O0FIX -c bitari -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O0SORT.ari
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT% -m O0SORT -c ari -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O0SORT.ari32
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT% -m O0SORT -c ari32 -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O0SORT.ari64
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT% -m O0SORT -c ari64 -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O0SORT.bitari
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT% -m O0SORT -c bitari -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O0PAIR.ari
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT% -m O0PAIR -c ari -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O0PAIR.ari32
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT% -m O0PAIR -c ari32 -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O0PAIR.ari64
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT% -m O0PAIR -c ari64 -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O0PAIR.bitari
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT% -m O0PAIR -c bitari -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O1FPAQ.ari
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT% -m O1FPAQ -c ari -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O1FPAQ.ari32
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT% -m O1FPAQ -c ari32 -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O1FPAQ.ari64
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT% -m O1FPAQ -c ari64 -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O1FPAQ.bitari
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT% -m O1FPAQ -c bitari -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O3MIX.ari
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT% -m O3MIX -c ari -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O3MIX.ari32
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT% -m O3MIX -c ari32 -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O3MIX.ari64
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT% -m O3MIX -c ari64 -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"

@set FILEOUT=%FOLDER%\%FILE%.O3MIX.bitari
copy "%FILE%" "%FILEOUT%"
ArithmeticConsole.exe %OPT% -m O3MIX -c bitari -a "%FILEOUT%" "%FILEOUT%"
@del "%FILEOUT%"


@echo 
@echo --------------uncompressing -----------------
@echo 

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.o1.ari.ar"
comp /M "%FILE%" "%FILE%.o1.ari"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.o1.ari' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.o1.ari"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.o1.ari32.ar"
comp /M "%FILE%" "%FILE%.o1.ari32"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.o1.ari32' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.o1.ari32"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.o1.ari64.ar"
comp /M "%FILE%" "%FILE%.o1.ari64"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.o1.ari64' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.o1.ari64"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.o1.bitari.ar"
comp /M "%FILE%" "%FILE%.o1.bitari"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.o1.bitari' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.o1.bitari"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.o2.ari.ar"
comp /M "%FILE%" "%FILE%.o2.ari"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.o2.ari' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.o2.ari"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.o2.ari32.ar"
comp /M "%FILE%" "%FILE%.o2.ari32"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.o2.ari32' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.o2.ari32"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.o2.ari64.ar"
comp /M "%FILE%" "%FILE%.o2.ari64"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.o2.ari64' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.o2.ari64"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.o2.bitari.ar"
comp /M "%FILE%" "%FILE%.o2.bitari"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.o2.bitari' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.o2.bitari"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O0FIX.ari.ar"
comp /M "%FILE%" "%FILE%.O0FIX.ari"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O0FIX.ari' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O0FIX.ari"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O0FIX.ari32.ar"
comp /M "%FILE%" "%FILE%.O0FIX.ari32"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O0FIX.ari32' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O0FIX.ari32"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O0FIX.ari64.ar"
comp /M "%FILE%" "%FILE%.O0FIX.ari64"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O0FIX.ari64' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O0FIX.ari64"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O0FIX.bitari.ar"
comp /M "%FILE%" "%FILE%.O0FIX.bitari"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O0FIX.bitari' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O0FIX.bitari"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O0SORT.ari.ar"
comp /M "%FILE%" "%FILE%.O0SORT.ari"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O0SORT.ari' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O0SORT.ari"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O0SORT.ari32.ar"
comp /M "%FILE%" "%FILE%.O0SORT.ari32"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O0SORT.ari32' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O0SORT.ari32"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O0SORT.ari64.ar"
comp /M "%FILE%" "%FILE%.O0FIX.ari64"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O0SORT.ari64' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O0SORT.ari64"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O0SORT.bitari.ar"
comp /M "%FILE%" "%FILE%.O0SORT.bitari"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O0SORT.bitari' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O0SORT.bitari"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O0PAIR.ari.ar"
comp /M "%FILE%" "%FILE%.O0PAIR.ari"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O0PAIR.ari' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O0PAIR.ari"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O0PAIR.ari32.ar"
comp /M "%FILE%" "%FILE%.O0PAIR.ari32"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O0PAIR.ari32' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O0PAIR.ari32"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O0PAIR.ari64.ar"
comp /M "%FILE%" "%FILE%.O0PAIR.ari64"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O0PAIR.ari64' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O0PAIR.ari64"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O0PAIR.bitari.ar"
comp /M "%FILE%" "%FILE%.O0PAIR.bitari"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O0PAIR.bitari' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O0PAIR.bitari"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O1FPAQ.ari.ar"
comp /M "%FILE%" "%FILE%.O1FPAQ.ari"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O1FPAQ.ari' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O1FPAQ.ari"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O1FPAQ.ari32.ar"
comp /M "%FILE%" "%FILE%.O1FPAQ.ari32"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O1FPAQ.ari32' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O1FPAQ.ari32"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O1FPAQ.ari64.ar"
comp /M "%FILE%" "%FILE%.O1FPAQ.ari64"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O1FPAQ.ari64' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O1FPAQ.ari64"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O1FPAQ.bitari.ar"
comp /M "%FILE%" "%FILE%.O1FPAQ.bitari"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O1FPAQ.bitari' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O1FPAQ.bitari"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O3MIX.ari.ar"
comp /M "%FILE%" "%FILE%.O3MIX.ari"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O3MIX.ari' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O3MIX.ari"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O3MIX.ari32.ar"
comp /M "%FILE%" "%FILE%.O3MIX.ari32"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O3MIX.ari32' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O3MIX.ari32"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O3MIX.ari64.ar"
comp /M "%FILE%" "%FILE%.O3MIX.ari64"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O3MIX.ari64' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O3MIX.ari64"

ArithmeticConsole.exe -x "%FOLDER%\%FILE%.O3MIX.bitari.ar"
comp /M "%FILE%" "%FILE%.O3MIX.bitari"
@if errorlevel 1 echo FILES '%FILE%' and '%FILE%.O3MIX.bitari' ARE NOT EQUAL
@if errorlevel 0 del "%FILE%.O3MIX.bitari"

@echo --------------done -----------------

