GitHub upload instructions — FreshFold

This file explains how to create a single archive of the project and upload it to GitHub manually (via the web UI) or push via git. Follow the steps below.

1) Prepare the project (local checks)
- Open a terminal in the project root:
  C:\Users\user\Documents\FreshFold\FreshFold
- Confirm the repo contains only intended files:
  git status --porcelain
- Ensure runtime secrets are not included (the file must not be uploaded):
  - `config/database.ini` should NOT be present in the upload. Keep `config/database.example.ini`.

Files and folders that MUST be included in the upload:
- README.md
- LICENSE
- config/database.example.ini
- database/freshfold_db.sql
- database/sample_data.sql
- src/ (all .cpp/.h files and subfolders)
- lib/mysql/README.txt (do NOT commit binary connector DLLs unless you intend to)
- docs/, tests/, screenshots/ (optional)
- publish_to_github.bat, create_release.bat

Files/folders that MUST NOT be uploaded:
- x64/ (build outputs)
- config/database.ini (local secret)
- lib/mysql/*.dll (optional; these are large binaries and better provided separately)

2) Create a zip archive (PowerShell)
- From the project root run (this excludes build outputs and the runtime config listed in .gitignore):

# create a clean list of tracked files and archive them (recommended)
git archive --format=zip -o ..\FreshFold-upload.zip HEAD

This creates `FreshFold-upload.zip` in the parent folder. The archive contains tracked files from the current commit.

If you do not use git commits, use PowerShell Compress-Archive and exclude patterns:

$source = "${PWD}"
$destination = "${PWD}\..\FreshFold-upload.zip"
$exclude = @('x64','bin','obj','config\\database.ini')
Get-ChildItem -Path $source -Recurse | Where-Object { $exclude -notcontains ($_.FullName -replace [regex]::Escape($source+"\\"), '') } | Compress-Archive -DestinationPath $destination

3) Upload the zip via GitHub web (manual)
- Open https://github.com/gamagemandira-ship-it/Fresh-Fold
- If the repository already exists and you want to upload files directly:
  - Click "Add file" -> "Upload files" and drag the unzipped contents into the browser window, or
  - Unzip `FreshFold-upload.zip` locally and drag the folder contents to the upload area.
- Commit the upload using the provided form.

4) Better: push using git (recommended)
- If you can authenticate with GitHub, pushing preserves history. Use:
  git add -A
  git commit -m "Publish FreshFold project"
  git branch -M main
  git remote set-url origin https://github.com/gamagemandira-ship-it/Fresh-Fold.git
  git push -u origin main

If push fails due to a protected or different remote history, create a new branch and push:
  git checkout -b publish
  git push -u origin publish

5) After upload
- On GitHub check the Files list and README.
- Do NOT expose `config/database.ini` or real passwords. If you accidentally pushed a secret, remove it and rotate the credential immediately.

If you prefer, run `create_release.bat` in the project root to create `..\FreshFold-release.zip`. Upload that zip the same way.

If any step fails, paste the exact terminal output here and I will provide the exact corrective command.
