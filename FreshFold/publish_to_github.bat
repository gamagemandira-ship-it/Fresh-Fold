@echo off
REM Edit the REMOTE_URL variable below to match your repository URL.
set REMOTE_URL=https://github.com/gamagemandira-ship-it/Fresh-Fold.git

n:: Initialize repo if needed, add, commit and push to remote
if not exist .git (
    git init
)

git remote remove origin 2>nul
if not exist .git (goto add_and_push)

:ensure_branch
for /f "tokens=2" %%b in ('git rev-parse --abbrev-ref HEAD') do set CURBR=%%b
if "%CURBR%"=="main" goto add_and_push
if "%CURBR%"=="master" (
    git branch -M main
) else (
    git branch -M main
)

:add_and_push
git add .
git commit -m "Initial import of FreshFold project" || echo "No changes to commit"
git remote add origin %REMOTE_URL% 2>nul || git remote set-url origin %REMOTE_URL%
git push -u origin main

echo Done. If push failed, authenticate with GitHub and run this script again.
pause
