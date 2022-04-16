@echo off

:: This batch file will generate a C HEADER file containing formatted 'git log' information.
:: It is intended to be used as pre build script to embed git log into during compilation time.

echo Running pre build script...

:: globals
set GIT_FOLDER=.git\
set OUTPUT_FILE=git_log.h

:: workaround to get to root folder
if NOT exist %GIT_FOLDER% (
    cd ..
    if NOT exist %GIT_FOLDER% (    
        echo Error! No .git repo found in this folder, aborting...
        goto :eof
    )
)

:: clean old files
if exist %OUTPUT_FILE% (    
    rm %OUTPUT_FILE%
)

:: gather git log data
FOR /F "tokens=*" %%g IN ('git config --get remote.origin.url') DO SET GIT_URL=%%g
FOR /F "tokens=*" %%g IN ('git log -1 --pretty^=\"%%h\"') DO SET GIT_COMMIT_ID=%%g
FOR /F "tokens=*" %%g IN ('git log -1 --pretty^=\"%%D\"') DO SET GIT_BRANCH=%%g
FOR /F "tokens=*" %%g IN ('git log -1 --pretty^=\"%%as\"') DO SET AUTHOR_EMAIL=%%g
FOR /F "tokens=*" %%g IN ('git log -1 --pretty^=\"%%ah\"') DO SET GIT_COMMIT_DATETIME=%%g
FOR /F "tokens=*" %%g IN ('git log -1 --pretty^=\"%%<(100,trunc)%%s\"') DO SET GIT_COMMIT_MSG=%%g

:: remove any double quotes from git message (if exists)
set GIT_COMMIT_MSG="%GIT_COMMIT_MSG:"=%"

:: build file structure
set H_HEADER=//! =============== Auto-generated file, do not edit. ===============
set GIT_LINE_1=#define GIT_LINE_1 "%GIT_COMMIT_ID:"=% (%GIT_BRANCH:"=%) %AUTHOR_EMAIL:"=% %GIT_COMMIT_DATETIME:"=%"
set GIT_LINE_2=#define GIT_LINE_2 "%GIT_COMMIT_MSG:"=%"
set GIT_URL=#define GIT_URL "%GIT_URL:"=%"

echo Generating %OUTPUT_FILE% ...

:: write output file
echo %H_HEADER% >> %OUTPUT_FILE%
echo, >> %OUTPUT_FILE%
echo %GIT_LINE_1% >> %OUTPUT_FILE%
echo %GIT_LINE_2% >> %OUTPUT_FILE%
echo %GIT_URL% >> %OUTPUT_FILE%

echo All Done!

:: exit
goto :eof
