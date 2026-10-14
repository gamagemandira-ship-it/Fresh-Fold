Put the MySQL Connector/NET DLL files in this folder.

1. Install MySQL Connector/NET (Windows installer) from
   https://dev.mysql.com/downloads/connector/net/
2. Open its installation folder, normally
   C:\Program Files (x86)\MySQL\MySQL Connector NET <version>\
3. In its "Assemblies" folder, open the folder for .NET Framework 4.8
   (or the closest .NET Framework 4.x folder).
4. Copy ALL the .dll files from that folder into this folder
   (MySql.Data.dll and the libraries it depends on).

The DLL files are ignored by Git (see .gitignore); every team member copies
them once. The build copies them next to FreshFold.exe automatically.
