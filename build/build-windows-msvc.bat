@echo off
setlocal
cd /d "%~dp0.."
where cl >nul 2>nul
if errorlevel 1 (
  echo cl.exe not found. Run this from a Visual Studio Developer Command Prompt.
  pause
  exit /b 1
)
rem Layout: see build/sources.sh. The resource script adds the icon and version details.
rc /nologo /Isrc /fo tinyshell.res src\platform\win32\tinyshell.rc
if errorlevel 1 exit /b 1
rem SQLite on its own, without warnings (not our code); flags as SQLITE_FLAGS in build/sources.sh.
cl /nologo /O1 /MT /W0 /c /DSQLITE_THREADSAFE=0 /DSQLITE_DEFAULT_MEMSTATUS=0 /DSQLITE_OMIT_LOAD_EXTENSION /DSQLITE_OMIT_DEPRECATED /DSQLITE_OMIT_SHARED_CACHE /DSQLITE_OMIT_PROGRESS_CALLBACK /DSQLITE_OMIT_AUTHORIZATION /DSQLITE_OMIT_TRACE /DSQLITE_OMIT_COMPLETE /DSQLITE_OMIT_GET_TABLE /DSQLITE_OMIT_TCL_VARIABLE /DSQLITE_OMIT_UTF16 /DSQLITE_DQS=0 /DSQLITE_LIKE_DOESNT_MATCH_BLOBS /DSQLITE_MAX_EXPR_DEPTH=0 /DSQLITE_OMIT_DECLTYPE /DSQLITE_OMIT_JSON /DSQLITE_WIN32_NO_ANSI vendor\sqlite\sqlite3.c
if errorlevel 1 exit /b 1
cl /nologo /O1 /MT /W4 /Isrc /Isrc\core /Isrc\app /Isrc\platform /Isrc\platform\win32 /Ivendor\sqlite src\core\*.c src\app\*.c src\platform\win32\*.c sqlite3.obj tinyshell.res winhttp.lib crypt32.lib advapi32.lib user32.lib shell32.lib /Fe:tinyshell.exe
if errorlevel 1 exit /b 1
echo Built tinyshell.exe
endlocal
