@echo off
rem CR3A installer: copies the VST3 into the standard folder FL Studio scans.
rem Right click -> "Run as administrator" if the copy is refused.
setlocal
set "TARGET=%CommonProgramFiles%\VST3"
if not exist "%TARGET%" mkdir "%TARGET%"
xcopy /E /I /Y "%~dp0CR3A.vst3" "%TARGET%\CR3A.vst3"
if errorlevel 1 (
  echo.
  echo Copy failed. Start this file as administrator.
  pause
  exit /b 1
)
echo.
echo Done. In FL Studio: Options ^> Manage plugins ^> Find installed plugins (Verify plugins)
pause
