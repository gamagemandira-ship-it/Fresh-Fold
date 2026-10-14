#pragma once

// ============================================================================
// StringUtils.h
// Conversions between .NET strings (System::String^, used by Windows Forms
// and MySQL Connector/NET) and standard C++ strings (std::string, used by
// the model classes). UTF-8 is used so names with Sinhala/Tamil letters are
// not lost.
// Only include this from files compiled with /clr.
// ============================================================================

#include <string>

namespace FreshFold
{
    inline std::string toStdString(System::String^ text)
    {
        if (text == nullptr || text->Length == 0)
            return std::string();

        array<System::Byte>^ bytes = System::Text::Encoding::UTF8->GetBytes(text);
        pin_ptr<System::Byte> pinned = &bytes[0];   // stops the garbage collector moving the array
        unsigned char* raw = pinned;
        return std::string(reinterpret_cast<const char*>(raw), static_cast<std::size_t>(bytes->Length));
    }

    inline System::String^ toManagedString(const std::string& text)
    {
        if (text.empty())
            return System::String::Empty;

        array<System::Byte>^ bytes = gcnew array<System::Byte>(static_cast<int>(text.size()));
        System::Runtime::InteropServices::Marshal::Copy(
            System::IntPtr(const_cast<char*>(text.data())), bytes, 0, bytes->Length);
        return System::Text::Encoding::UTF8->GetString(bytes);
    }

    // Same as toManagedString, but converts "\n" line breaks to Windows "\r\n"
    // so multi-line text displays correctly in a TextBox.
    inline System::String^ toDisplayText(const std::string& text)
    {
        return toManagedString(text)->Replace("\r\n", "\n")->Replace("\n", System::Environment::NewLine);
    }
}
