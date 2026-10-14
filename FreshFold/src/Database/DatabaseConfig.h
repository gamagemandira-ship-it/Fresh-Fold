#pragma once

// ============================================================================
// DatabaseConfig.h
// Holds the MySQL connection settings read from config/database.ini.
// The real database.ini is NOT committed to GitHub (see .gitignore);
// config/database.example.ini shows the expected format.
// ============================================================================

#include <string>

namespace FreshFold
{
    struct DatabaseConfig
    {
        std::string server = "localhost";
        int port = 3306;
        // Default database name. Set to `fresh_fold` to match requested database.
        std::string database = "fresh_fold";
        std::string user = "root";
        std::string password = "@Mani#2004#";

        // Reads an INI file such as:
        //   [mysql]
        //   server=localhost
        //   port=3306
        //   database=freshfold_db
        //   user=freshfold_app
        //   password=...
        // Throws std::runtime_error if the text is incomplete or invalid.
        static DatabaseConfig parse(const std::string& iniText);

        // Reads the file and calls parse(). Throws std::runtime_error if the
        // file cannot be opened. (The GUI reads the file with .NET instead, so
        // folder names with non-English characters also work, then calls parse().)
        static DatabaseConfig loadFromFile(const std::string& path);
    };
}
