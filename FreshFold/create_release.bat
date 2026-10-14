@echo off
REM Creates a zip archive of the current Git HEAD (tracked files only).
REM Run this from the project root: C:\Users\user\Documents\FreshFold\FreshFold

n:: Ensure repository is up to date locally (optional)
git add -A
git commit -m "Release snapshot" 2>nul || echo "No new changes to commit"

n:: Create archive one level above the project folder
git archive -o "%~dp0..\FreshFold-release.zip" HEAD
if %ERRORLEVEL% EQU 0 (
  echo Archive created: %~dp0..\FreshFold-release.zip
) else (
  echo Failed to create archive. Ensure Git is installed and this folder is a git repo.
)
pause
