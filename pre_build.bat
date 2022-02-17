@echo off

:: This batch file will generate a C HEADER file containing formatted 'git log' information.
:: It is intended to be used as pre build script to embed git log into during compilation time.

echo Running pre build script...

:: globals
set GIT_FOLDER=.git\
set OUTPUT_FILE=git_log.h
set GIT_URL_TEMP=""

:: output file structure
set H_HEADER=//! =============== Auto-generated file, do not edit. ===============
::set H_BODY="#ifndef GIT_INFO_PRESENT%%n#define GIT_INFO_PRESENT%%n%%nstatic const char* git_line_1 = \"%%h (%%D) by %%aE %%as %%ah\";%%nstatic const char* git_line_2 = %%<(80,trunc)\"%%s\";"
set H_BODY="#ifndef GIT_INFO_PRESENT%%n#define GIT_INFO_PRESENT%%n%%n#define GIT_LINE_1 \"%%h (%%D) by %%aE %%as %%ah\"%%n#define GIT_LINE_2 %%<(80,trunc)\"%%s\""

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

:: get repo url (if not set it will be "")
FOR /F "tokens=*" %%g IN ('git config --get remote.origin.url') DO SET GIT_URL_TEMP=%%g

:: generate output file
echo Generating %C_HEADER_OUTPUT_FILE% ...

echo %H_HEADER% >> %OUTPUT_FILE%
echo, >> %OUTPUT_FILE%
git log -1 --decorate=full --decorate-refs-exclude='refs/heads' --pretty=%H_BODY% >> %OUTPUT_FILE%
echo #define GIT_URL "%GIT_URL_TEMP%" >> %OUTPUT_FILE%
echo #endif >> %OUTPUT_FILE%
echo, >> %OUTPUT_FILE%

echo pre_build... All Done!

:: exit
goto :eof


:: backup stuff
::set C_HEADER_TEMPLATE="#ifndef GIT_INFO_PRESENT%%n#define GIT_INFO_PRESENT%%nstatic const char* git_info_commit_id = \"%%C(bold green)%%h %%C(reset)%%C(bold)%%D %%C(reset) Author: %%aN %%ce, %%ch [%%ar]\";%%nstatic const char* git_commit_comment = %%<(80,trunc)\"%%s\";"
::git log --decorate=full --decorate-refs-exclude='refs/heads' --pretty=format:'%C(bold green)%h %C(reset)%C(bold)%D %C(reset) Author: %aN %ce, %ch [%ar]%n commit msg: %s'
::get system date and time
::set COMPILATION_TIME=%DATE%%TIME%
::set COMPILATION_TIME=%COMPILATION_TIME:~0,-3%
::set GIT_URL_TEMP=""