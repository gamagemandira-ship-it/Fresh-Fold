#include "DatabaseConfig.h"
#include "../Models/Customer.h"   // for Customer::trim

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace FreshFold
{
    DatabaseConfig DatabaseConfig::loadFromFile(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
            throw std::runtime_error("Could not open the database configuration file:\n" + path);

        std::ostringstream content;
        content << file.rdbuf();
        return parse(content.str());
    }

    DatabaseConfig DatabaseConfig::parse(const std::string& iniText)
    {
        std::istringstream file(iniText);
        DatabaseConfig config;
        std::string line;
        bool firstLine = true;
        while (std::getline(file, line))
        {
            // Notepad may save the file with a UTF-8 "BOM" marker; ignore it.
            if (firstLine && line.size() >= 3 && line.compare(0, 3, "\xEF\xBB\xBF") == 0)
                line = line.substr(3);
            firstLine = false;

            line = Customer::trim(line);

            // Skip blank lines, comments and section headers like [mysql].
            if (line.empty() || line[0] == ';' || line[0] == '#' || line[0] == '[')
                continue;

            std::string::size_type equals = line.find('=');
            if (equals == std::string::npos)
                continue;

            std::string key = Customer::trim(line.substr(0, equals));
            std::string value = Customer::trim(line.substr(equals + 1));

            if (key == "server") config.server = value;
            else if (key == "database") config.database = value;
            else if (key == "user") config.user = value;
            else if (key == "password") config.password = value;
            else if (key == "port")
            {
                try
                {
                    config.port = std::stoi(value);
                }
                catch (const std::exception&)
                {
                    throw std::runtime_error("The 'port' value in the database configuration file is not a number.");
                }
            }
        }

        if (config.server.empty() || config.database.empty() || config.user.empty())
            throw std::runtime_error("The database configuration file must contain server, database and user values.");
        if (config.port <= 0 || config.port > 65535)
            throw std::runtime_error("The 'port' value in the database configuration file is out of range.");

        return config;
    }
}
