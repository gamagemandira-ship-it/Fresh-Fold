#include "AuthenticationService.h"

#include "AuthenticationService.h"
#include <stdexcept>

using namespace System;
using namespace System::Security::Cryptography;

namespace FreshFold
{
    AuthenticationService::AuthenticationService(const DatabaseManager& database)
        : database(database)
    {
    }

    bool AuthenticationService::isFirstRun() const
    {
        return database.countUsers() == 0;
    }

    bool AuthenticationService::login(String^ username, String^ password) const
    {
        if (String::IsNullOrWhiteSpace(username) || String::IsNullOrEmpty(password))
            return false;

        // Parameterized query inside DatabaseManager - safe against SQL injection.
        String^ storedHash = database.getPasswordHash(username->Trim());
        if (storedHash == nullptr)
            return false;   // unknown user (the GUI shows the same message as a wrong password)

        return verifyPassword(password, storedHash);
    }

    void AuthenticationService::createInitialAdmin(String^ username, String^ password, String^ confirmPassword) const
    {
        String^ name = (username == nullptr) ? String::Empty : username->Trim();

        if (name->Length < 3 || name->Length > 50)
            throw std::invalid_argument("The username must be between 3 and 50 characters.");
        for each (wchar_t ch in name)
        {
            if (!Char::IsLetterOrDigit(ch) && ch != L'_' && ch != L'.' && ch != L'-')
                throw std::invalid_argument("The username may only contain letters, digits, '.', '_' and '-'.");
        }
        if (password == nullptr || password->Length < 8)
            throw std::invalid_argument("The password must be at least 8 characters long.");
        if (password->Length > 128)
            throw std::invalid_argument("The password must be 128 characters or fewer.");
        if (!String::Equals(password, confirmPassword))
            throw std::invalid_argument("The two passwords do not match.");

        if (!database.createFirstUser(name, hashPassword(password)))
            throw std::invalid_argument("An administrator account already exists. Please log in instead.");
    }

    String^ AuthenticationService::hashPassword(String^ password)
    {
        // A new random salt for every password, so equal passwords get different hashes.
        array<Byte>^ salt = gcnew array<Byte>(SALT_BYTES);
        RandomNumberGenerator^ random = RandomNumberGenerator::Create();
        random->GetBytes(salt);
        delete random;   // calls Dispose()

        Rfc2898DeriveBytes pbkdf2(password, salt, HASH_ITERATIONS, HashAlgorithmName::SHA256);
        array<Byte>^ hash = pbkdf2.GetBytes(HASH_BYTES);

        return String::Format("PBKDF2-SHA256${0}${1}${2}",
                              HASH_ITERATIONS, Convert::ToBase64String(salt), Convert::ToBase64String(hash));
    }

    bool AuthenticationService::verifyPassword(String^ password, String^ storedHash)
    {
        if (password == nullptr || storedHash == nullptr)
            return false;

        array<String^>^ parts = storedHash->Split(gcnew array<wchar_t>{ L'$' });
        if (parts->Length != 4 || !String::Equals(parts[0], "PBKDF2-SHA256"))
            return false;

        int iterations = 0;
        if (!Int32::TryParse(parts[1], iterations) || iterations < 10000)
            return false;

        array<Byte>^ salt = nullptr;
        array<Byte>^ expectedHash = nullptr;
        try
        {
            salt = Convert::FromBase64String(parts[2]);
            expectedHash = Convert::FromBase64String(parts[3]);
        }
        catch (FormatException^)
        {
            return false;   // damaged hash in the database
        }
        if (salt->Length < 8 || expectedHash->Length < 16)
            return false;

        // Hash the entered password with the SAME salt and iteration count.
        Rfc2898DeriveBytes pbkdf2(password, salt, iterations, HashAlgorithmName::SHA256);
        array<Byte>^ actualHash = pbkdf2.GetBytes(expectedHash->Length);

        // Compare every byte (no early exit) so the time taken does not reveal
        // how many bytes matched.
        int difference = 0;
        for (int i = 0; i < expectedHash->Length; i++)
            difference |= actualHash[i] ^ expectedHash[i];
        return difference == 0;
    }
}
