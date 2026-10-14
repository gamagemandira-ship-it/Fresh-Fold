#pragma once

// ============================================================================
// UiHelpers.h
// Small helper functions so every screen uses the same colours, fonts and
// control styles, and so the form code is not full of repeated settings.
// The forms are written in code (not with the Visual Studio designer), which
// keeps them easy to read and avoids designer problems with C++/CLI.
// ============================================================================

namespace FreshFold
{
    using namespace System;
    using namespace System::Drawing;
    using namespace System::Windows::Forms;

    public ref class Ui abstract sealed
    {
    public:
        // ---------- Colours ----------
        static Color Navy() { return Color::FromArgb(31, 58, 95); }
        static Color NavyLight() { return Color::FromArgb(45, 80, 125); }
        static Color Background() { return Color::FromArgb(244, 246, 249); }
        static Color SuccessGreen() { return Color::FromArgb(25, 125, 60); }
        static Color ErrorRed() { return Color::FromArgb(185, 35, 35); }

        static System::Drawing::Font^ MakeFont(float size, bool bold)
        {
            return gcnew System::Drawing::Font("Segoe UI", size, bold ? FontStyle::Bold : FontStyle::Regular);
        }

        // Common settings for every window.
        static void StyleForm(Form^ form, String^ title, int width, int height)
        {
            form->Text = String::Concat("FreshFold - ", title);
            form->ClientSize = System::Drawing::Size(width, height);
            form->BackColor = Background();
            form->Font = MakeFont(9.75f, false);
            form->StartPosition = FormStartPosition::CenterScreen;
            form->FormBorderStyle = FormBorderStyle::FixedSingle;
            form->MaximizeBox = false;
        }

        // Dark blue title bar at the top of a screen.
        static Panel^ MakeHeader(String^ title, String^ subtitle)
        {
            Panel^ header = gcnew Panel();
            header->Dock = DockStyle::Top;
            header->Height = 64;
            header->BackColor = Navy();

            Label^ titleLabel = gcnew Label();
            titleLabel->Text = title;
            titleLabel->ForeColor = Color::White;
            titleLabel->Font = MakeFont(16.0f, true);
            titleLabel->AutoSize = true;
            titleLabel->Location = Point(20, 8);
            header->Controls->Add(titleLabel);

            Label^ subtitleLabel = gcnew Label();
            subtitleLabel->Text = subtitle;
            subtitleLabel->ForeColor = Color::FromArgb(200, 215, 235);
            subtitleLabel->AutoSize = true;
            subtitleLabel->Location = Point(22, 40);
            header->Controls->Add(subtitleLabel);

            return header;
        }

        static Label^ MakeLabel(String^ text, int x, int y)
        {
            Label^ label = gcnew Label();
            label->Text = text;
            label->AutoSize = true;
            label->Location = Point(x, y + 3);
            return label;
        }

        static TextBox^ MakeTextBox(int x, int y, int width)
        {
            TextBox^ box = gcnew TextBox();
            box->Location = Point(x, y);
            box->Width = width;
            return box;
        }

        static ComboBox^ MakeDropDown(int x, int y, int width)
        {
            ComboBox^ combo = gcnew ComboBox();
            combo->DropDownStyle = ComboBoxStyle::DropDownList;   // user can only pick listed values
            combo->Location = Point(x, y);
            combo->Width = width;
            return combo;
        }

        // primary = dark blue button for the main action, otherwise a plain button.
        static Button^ MakeButton(String^ text, int x, int y, int width, bool primary)
        {
            Button^ button = gcnew Button();
            button->Text = text;
            button->Location = Point(x, y);
            button->Size = System::Drawing::Size(width, 32);
            button->Cursor = Cursors::Hand;
            if (primary)
            {
                button->FlatStyle = FlatStyle::Flat;
                button->FlatAppearance->BorderSize = 0;
                button->BackColor = Navy();
                button->ForeColor = Color::White;
            }
            else
            {
                button->UseVisualStyleBackColor = true;
            }
            return button;
        }

        // Read-only table with whole-row selection.
        static DataGridView^ MakeGrid(int x, int y, int width, int height)
        {
            DataGridView^ grid = gcnew DataGridView();
            grid->Location = Point(x, y);
            grid->Size = System::Drawing::Size(width, height);
            grid->ReadOnly = true;
            grid->AllowUserToAddRows = false;
            grid->AllowUserToDeleteRows = false;
            grid->AllowUserToResizeRows = false;
            grid->MultiSelect = false;
            grid->SelectionMode = DataGridViewSelectionMode::FullRowSelect;
            grid->AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode::Fill;
            grid->RowHeadersVisible = false;
            grid->BackgroundColor = Color::White;
            grid->BorderStyle = BorderStyle::FixedSingle;
            grid->EnableHeadersVisualStyles = false;
            grid->ColumnHeadersDefaultCellStyle->BackColor = NavyLight();
            grid->ColumnHeadersDefaultCellStyle->ForeColor = Color::White;
            grid->ColumnHeadersDefaultCellStyle->Font = MakeFont(9.75f, true);
            grid->AlternatingRowsDefaultCellStyle->BackColor = Color::FromArgb(240, 244, 250);
            return grid;
        }

        // Applies a number format (e.g. "N2") to a grid column if it exists.
        static void FormatColumn(DataGridView^ grid, String^ columnName, String^ format)
        {
            if (grid->Columns->Contains(columnName))
            {
                grid->Columns[columnName]->DefaultCellStyle->Format = format;
                grid->Columns[columnName]->DefaultCellStyle->Alignment = DataGridViewContentAlignment::MiddleRight;
            }
        }

        // Green (success) or red (error) message under the buttons.
        static void SetStatus(Label^ label, String^ message, bool success)
        {
            label->Text = message;
            label->ForeColor = success ? SuccessGreen() : ErrorRed();
        }

        static void ShowError(String^ message)
        {
            MessageBox::Show(message, "FreshFold", MessageBoxButtons::OK, MessageBoxIcon::Warning);
        }

        static void ShowInfo(String^ message)
        {
            MessageBox::Show(message, "FreshFold", MessageBoxButtons::OK, MessageBoxIcon::Information);
        }

        static bool Confirm(String^ message)
        {
            return MessageBox::Show(message, "FreshFold - please confirm", MessageBoxButtons::YesNo,
                                    MessageBoxIcon::Question, MessageBoxDefaultButton::Button2)
                   == DialogResult::Yes;
        }
    };
}
