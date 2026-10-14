#pragma once

// ============================================================================
// AuthenticationService.h
// Login logic, kept separate from the login screen.
//
// Passwords are NEVER stored. Only a salted hash is stored, created with
// PBKDF2 (HMAC-SHA256, 100,000 iterations) using the .NET Framework class
// System::Security::Cryptography::Rfc2898DeriveBytes - a maintained,
// standard algorithm. No home-made cryptography is used.
//
// Stored format:  PBKDF2-SHA256$<iterations>$<salt in Base64>$<hash in Base64>
// ============================================================================

#include "../Database/DatabaseManager.h"

namespace FreshFold
{
    class AuthenticationService
    {
    private:
        const DatabaseManager& database;

        static const int HASH_ITERATIONS = 100000;
        static const int SALT_BYTES = 16;
        static const int HASH_BYTES = 32;

    public:
        explicit AuthenticationService(const DatabaseManager& database);

        // True when the users table is empty (the first administrator must be created).
        bool isFirstRun() const;

        // True if the username exists and the password matches its stored hash.
        // Throws DatabaseException if MySQL cannot be reached.
        bool login(System::String^ username, System::String^ password) const;

        // First-run setup. Throws std::invalid_argument for invalid input or
        // if an account already exists.
        void createInitialAdmin(System::String^ username, System::String^ password,
                                System::String^ confirmPassword) const;

        static System::String^ hashPassword(System::String^ password);
        static bool verifyPassword(System::String^ password, System::String^ storedHash);
    };
}
