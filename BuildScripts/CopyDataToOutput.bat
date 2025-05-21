@echo off

set SOURCE=%1
set DEST=%2

xcopy %SOURCE%\Data %DEST%\Data /e /i /y