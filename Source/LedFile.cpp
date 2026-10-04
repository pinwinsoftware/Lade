#include "LedFile.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <windows.h>
#include <string>

bool UpdateFile(const std::string& ledPath, const std::string& fileName, const std::vector<char>& newData) {
    // Open existing LED file
    std::ifstream led(ledPath, std::ios::binary);

    if (!led)
        return false;

    // Read header
    LedHeader header{};

    led.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (!led)
        return false;

    // Check LED signature
    if (header.magic[0] != 'L' ||
        header.magic[1] != 'E' ||
        header.magic[2] != 'D' ||
        header.magic[3] != '1') {
        return false;
    }

    // Read all file entries first
    std::vector<LedFileEntry> entries;

    for (uint32_t i = 0; i < header.fileCount; i++) {
        LedFileEntry entry{};

        led.read(reinterpret_cast<char*>(&entry), sizeof(entry));

        if (!led)
            return false;

        entries.push_back(entry);
    }

    // Read all file data
    std::vector<std::vector<char>> fileData;

    fileData.reserve(entries.size());

    bool found = false;

    for (const LedFileEntry& entry : entries) {
        std::vector<char> data(entry.size);

        // Go to this file's data
        led.clear();

        led.seekg(entry.offset, std::ios::beg);

        if (!led)
            return false;

        if (entry.size > 0) {
            led.read(data.data(), entry.size);

            if (!led)
                return false;
        }

        // Replace the target file's data
        if (fileName == entry.name) {
            data = newData;
            found = true;
        }

        fileData.push_back(std::move(data));
    }

    led.close();

    if (!found)
        return false;

    // Update sizes
    for (size_t i = 0; i < entries.size(); i++) {
        entries[i].size = static_cast<uint32_t>(fileData[i].size());
    }

    // Calculate new data offsets
    uint32_t dataOffset = sizeof(LedHeader) + static_cast<uint32_t>(entries.size() * sizeof(LedFileEntry));

    for (auto& entry : entries) {
        entry.offset = dataOffset;

        dataOffset += entry.size;
    }

    // Temporary LED file
    std::string tempPath = ledPath + ".tmp";

    std::ofstream output(tempPath, std::ios::binary | std::ios::trunc);

    if (!output)
        return false;

    // New header
    LedHeader newHeader{};

    newHeader.magic[0] = 'L';
    newHeader.magic[1] = 'E';
    newHeader.magic[2] = 'D';
    newHeader.magic[3] = '1';

    newHeader.fileCount = static_cast<uint32_t>(entries.size());

    // Write header
    output.write(reinterpret_cast<const char*>(&newHeader), sizeof(newHeader));

    if (!output) {
        output.close();
        std::remove(tempPath.c_str());
        return false;
    }

    // Write entries
    for (const auto& entry : entries) {
        output.write(reinterpret_cast<const char*>(&entry), sizeof(LedFileEntry));

        if (!output) {
            output.close();
            std::remove(tempPath.c_str());
            return false;
        }
    }

    // Write file data
    for (const auto& data : fileData) {
        if (!data.empty()) {
            output.write(
                data.data(),
                data.size()
            );

            if (!output) {
                output.close();
                std::remove(tempPath.c_str());
                return false;
            }
        }
    }

    // Close temporary file
    output.close();

    if (!output) {
        std::remove(tempPath.c_str());
        return false;
    }

    // Replace original
    std::string backupPath = ledPath + ".bak";

    std::remove(backupPath.c_str());

    if (std::rename(ledPath.c_str(), backupPath.c_str()) != 0) {
        std::remove(tempPath.c_str());
        return false;
    }

    if (std::rename(tempPath.c_str(), ledPath.c_str()) != 0) {
        std::rename(backupPath.c_str(), ledPath.c_str());
        std::remove(tempPath.c_str());

        return false;
    }

    // Delete backup
    std::remove(backupPath.c_str());

    return true;
}

bool IncludeFile(const std::string& ledPath, const std::string& sourcePath) {
    
    // Read the source file
    std::ifstream source(sourcePath, std::ios::binary);

    if (!source)
        return false;

    source.seekg(0, std::ios::end);

    std::streamsize fileSize = source.tellg();

    source.seekg(0, std::ios::beg);

    if (fileSize < 0)
        return false;

    std::vector<char> data(static_cast<size_t>(fileSize));

    if (fileSize > 0) {
        source.read(data.data(), fileSize);
    }

    source.close();

    // Open existing LED file
    std::ifstream oldLed(ledPath, std::ios::binary);

    if (!oldLed)
        return false;

    // Read header
    LedHeader header;

    oldLed.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.magic[0] != 'L' ||
        header.magic[1] != 'E' ||
        header.magic[2] != 'D' ||
        header.magic[3] != '1') {
        return false;
    }

    // Read existing entries
    std::vector<LedFileEntry> entries;

    for (uint32_t i = 0; i < header.fileCount; i++) {
        LedFileEntry entry{};

        oldLed.read(reinterpret_cast<char*>(&entry), sizeof(entry));

        if (!oldLed)
            return false;

        entries.push_back(entry);
    }

    // Read existing file data
    std::vector<std::vector<char>> fileData;

    for (const LedFileEntry& entry : entries) {
        std::vector<char> oldData(entry.size);

        oldLed.seekg(entry.offset, std::ios::beg);

        if (entry.size > 0){
            oldLed.read(oldData.data(),entry.size);
        }

        fileData.push_back(std::move(oldData));
    }

    oldLed.close();

    // Create new entry
    LedFileEntry newEntry{};

    std::string fileName = sourcePath;

    size_t slash = fileName.find_last_of("/\\");

    if (slash != std::string::npos) {
        fileName = fileName.substr(slash + 1);
    }

    strncpy_s(newEntry.name, fileName.c_str(), sizeof(newEntry.name) - 1);

    newEntry.size = static_cast<uint32_t>(fileSize);

    entries.push_back(newEntry);

    fileData.push_back(std::move(data));


    // Update header
    header.fileCount = static_cast<uint32_t>(entries.size());

    // Calculate data offsets
    uint32_t dataOffset = sizeof(LedHeader) + static_cast<uint32_t>(entries.size() * sizeof(LedFileEntry));

    for (size_t i = 0; i < entries.size(); i++) {
        entries[i].offset = dataOffset;

        dataOffset += entries[i].size;
    }

    // Rewrite LED file
    std::ofstream newLed(ledPath, std::ios::binary | std::ios::trunc);

    if (!newLed)
        return false;

    // Write header
    newLed.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Write all entries
    for (const LedFileEntry& entry : entries) {
        newLed.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
    }

    // Write all file data
    for (const auto& bytes : fileData) {
        if (!bytes.empty()) {
            newLed.write(bytes.data(), bytes.size());
        }
    }

    newLed.close();

    return true;
}

bool ExportFile(const std::string& ledPath, const std::string& fileName, const std::string& outputPath) {
    std::ifstream led(ledPath, std::ios::binary);

    if (!led)
        return false;

    // Read header
    LedHeader header;

    led.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.magic[0] != 'L' ||
        header.magic[1] != 'E' ||
        header.magic[2] != 'D' ||
        header.magic[3] != '1') {
        return false;
    }

    // Search for the file
    for (uint32_t i = 0; i < header.fileCount; i++) {
        LedFileEntry entry;

        led.read(reinterpret_cast<char*>(&entry), sizeof(entry));

        if (fileName == entry.name) {
            led.seekg(entry.offset, std::ios::beg);

            std::vector<char> data(entry.size);

            if (entry.size > 0) {
                led.read(data.data(), entry.size);

                if (!led)
                    return false;
            }

            // Create exported file
            std::ofstream output(outputPath, std::ios::binary);

            if (!output)
                return false;

            if (entry.size > 0) {
                output.write(data.data(), entry.size);

                if (!output)
                    return false;
            }

            output.close();

            if (!output)
                return false;

            return true;
        }
    }
    // File wasn't found
    return false;
}

bool CreateNewFile(const std::string& ledPath, const std::string& fileName) {
    std::ifstream led(ledPath, std::ios::binary);

    if (!led)
        return false;

    // Read header
    LedHeader header{};

    led.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (!led)
        return false;

    // Check LED signature
    if (header.magic[0] != 'L' ||
        header.magic[1] != 'E' ||
        header.magic[2] != 'D' ||
        header.magic[3] != '1') {
        return false;
    }

    // Check if file already exists
    std::vector<LedFileEntry> entries;

    for (uint32_t i = 0; i < header.fileCount; i++) {
        LedFileEntry entry{};

        led.read(reinterpret_cast<char*>(&entry), sizeof(entry));

        if (!led)
            return false;

        if (fileName == entry.name) {
            std::cout << "File \"" << fileName << "\" already exists\n";

            return false;
        }

        entries.push_back(entry);
    }

    led.close();

    // Create empty file data
    std::vector<char> emptyData;

    // Rebuild the LED file with the new entry
    struct StoredFile {
        LedFileEntry entry;
        std::vector<char> data;
    };

    std::vector<StoredFile> files;

    // Re-open LED so we can read the existing data
    std::ifstream input(ledPath, std::ios::binary);

    if (!input)
        return false;

    input.seekg(sizeof(LedHeader), std::ios::beg);

    for (const LedFileEntry& entry : entries) {
        StoredFile file;

        file.entry = entry;

        input.clear();
        input.seekg(entry.offset, std::ios::beg);

        if (!input)
            return false;

        file.data.resize(entry.size);

        if (entry.size > 0) {
            input.read(file.data.data(), entry.size);

            if (!input)
                return false;
        }

        files.push_back(std::move(file));
    }

    input.close();

    // Create the new empty entry
    StoredFile newFile{};

    strncpy_s(newFile.entry.name, sizeof(newFile.entry.name), fileName.c_str(), _TRUNCATE);

    newFile.entry.size = 0;

    files.push_back(std::move(newFile));

    // Create new header
    LedHeader newHeader{};

    newHeader.magic[0] = 'L';
    newHeader.magic[1] = 'E';
    newHeader.magic[2] = 'D';
    newHeader.magic[3] = '1';

    newHeader.fileCount = static_cast<uint32_t>(files.size());

    // Calculate data offsets
    uint32_t dataOffset = sizeof(LedHeader) + static_cast<uint32_t>(files.size() * sizeof(LedFileEntry));

    for (auto& file : files) {
        file.entry.offset = dataOffset;

        dataOffset += file.entry.size;
    }

    // Temporary file
    std::string tempPath = ledPath + ".tmp";

    std::ofstream output(tempPath, std::ios::binary | std::ios::trunc);

    if (!output)
        return false;

    // Write header
    output.write(reinterpret_cast<const char*>(&newHeader), sizeof(newHeader));

    if (!output) {
        output.close();
        std::remove(tempPath.c_str());
        return false;
    }

    // Write entries
    for (const auto& file : files) {
        output.write(reinterpret_cast<const char*>(&file.entry), sizeof(LedFileEntry));

        if (!output) {
            output.close();
            std::remove(tempPath.c_str());
            return false;
        }
    }

    // Write data
    for (const auto& file : files) {
        if (!file.data.empty()) {
            output.write(file.data.data(), file.data.size());

            if (!output) {
                output.close();
                std::remove(tempPath.c_str());
                return false;
            }
        }
    }

    output.close();

    if (!output) {
        std::remove(tempPath.c_str());
        return false;
    }

    // Replace original
    std::string backupPath = ledPath + ".bak";

    std::remove(backupPath.c_str());

    if (std::rename(ledPath.c_str(), backupPath.c_str()) != 0) {
        std::remove(tempPath.c_str());
        return false;
    }

    if (std::rename(tempPath.c_str(), ledPath.c_str()) != 0) {
        std::rename(backupPath.c_str(), ledPath.c_str());
        std::remove(tempPath.c_str());

        return false;
    }

    std::remove(backupPath.c_str());

    return true;
}

bool DeleteAssetFile(const std::string& ledPath, const std::string& targetName) {
    // Open the original LED file
    std::ifstream led(ledPath, std::ios::binary);

    if (!led)
        return false;

    // Read header
    LedHeader header{};

    led.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (!led)
        return false;

    // Check LED signature
    if (header.magic[0] != 'L' ||
        header.magic[1] != 'E' ||
        header.magic[2] != 'D' ||
        header.magic[3] != '1') {
        return false;
    }

    struct StoredFile {
        LedFileEntry entry;
        std::vector<char> data;
    };

    std::vector<StoredFile> files;

    bool found = false;

    // Read all files
    for (uint32_t i = 0; i < header.fileCount; i++) {
        LedFileEntry entry{};

        led.read(reinterpret_cast<char*>(&entry), sizeof(entry));

        if (!led)
            return false;

        // Save current position
        std::streampos dataPosition = led.tellg();

        // Go to the actual data
        led.seekg(entry.offset, std::ios::beg);

        StoredFile stored;
        stored.entry = entry;

        stored.data.resize(entry.size);

        if (entry.size > 0) {
            led.read(stored.data.data(), entry.size);

            if (!led)
                return false;
        }

        // Check whether this is the file to delete
        if (targetName == entry.name) {
            found = true;
        }
        else {
            files.push_back(std::move(stored));
        }

        // Return to where the next entry was stored
        led.clear();
        led.seekg(dataPosition, std::ios::beg);
    }

    led.close();

    if (!found)
        return false;

    // Temporary file
    std::string tempPath = ledPath + ".tmp";

    std::ofstream output(tempPath, std::ios::binary);

    if (!output)
        return false;

    // New header
    LedHeader newHeader{};

    newHeader.magic[0] = 'L';
    newHeader.magic[1] = 'E';
    newHeader.magic[2] = 'D';
    newHeader.magic[3] = '1';

    newHeader.fileCount = static_cast<uint32_t>(files.size());

    // Write header
    output.write(reinterpret_cast<const char*>(&newHeader), sizeof(newHeader));

    // We need to calculate where each file's data
    // will be located in the new LED file.
    uint32_t dataOffset = sizeof(LedHeader) + static_cast<uint32_t>(files.size() * sizeof(LedFileEntry));

    // Update offsets
    for (auto& file : files) {
        file.entry.offset = dataOffset;
        dataOffset += file.entry.size;
    }

    // Write all entries
    for (const auto& file : files) {
        output.write(reinterpret_cast<const char*>(&file.entry), sizeof(LedFileEntry));
    }

    // Write all data
    for (const auto& file : files) {
        if (!file.data.empty()) {
            output.write(file.data.data(), file.data.size());
        }
    }

    output.close();

    // Replace original file
    if (std::remove(ledPath.c_str()) != 0) {
        std::remove(tempPath.c_str());
        return false;
    }

    if (std::rename(tempPath.c_str(), ledPath.c_str()) != 0) {
        return false;
    }

    return true;
}

void ViewBinary(const std::string& ledPath, const std::string& targetName) {
    std::ifstream led(ledPath, std::ios::binary);

    if (!led) {
        std::cout << "Unable to open LED file\n";
        return;
    }

    // Read header
    LedHeader header;

    led.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.magic[0] != 'L' ||
        header.magic[1] != 'E' ||
        header.magic[2] != 'D' ||
        header.magic[3] != '1') {
        std::cout << "Invalid LED file\n";
        return;
    }

    LedFileEntry target{};
    bool found = false;

    for (uint32_t i = 0; i < header.fileCount; i++) {
        LedFileEntry entry{};

        led.read(reinterpret_cast<char*>(&entry), sizeof(entry));

        if (std::string(entry.name) == targetName) {
            target = entry;
            found = true;
            break;
        }
    }

    if (!found) {
        std::cout << "File not found\n";
        return;
    }

    // Go to the file's binary data
    led.clear();

    led.seekg(target.offset, std::ios::beg);

    if (!led) {
        std::cout << "Unable to read file data\n";
        return;
    }

    std::cout << "\n"; std::cout << "Binary: " << target.name << "\n";

    std::cout << "Size: " << target.size << " bytes\n\n";

    // Read and display bytes
    const int bytesPerLine = 16;

    for (uint32_t i = 0; i < target.size; i += bytesPerLine) {
        std::cout << std::hex << std::uppercase;

        // Address
        std::cout.width(8);
        std::cout.fill('0');
        std::cout << i;

        std::cout << "  ";

        // Hex bytes
        for (int j = 0; j < bytesPerLine; j++) {
            if (i + j < target.size) {
                unsigned char byte;

                led.read(reinterpret_cast<char*>(&byte), 1);

                std::cout.width(2);
                std::cout.fill('0');
                std::cout << static_cast<int>(byte) << " ";
            }
            else {
                std::cout << "   ";
            }
        }

        std::cout << "\n";

        // Limit output to 4096 bytes
        if (i >= 4096) {
            std::cout << "\n[Output limited to 4096 bytes]\n";
            break;
        }
    }

    std::cout << std::dec;
}

void ViewTextFile(const std::string& ledPath, const std::string& targetName) {
    std::ifstream led(ledPath, std::ios::binary);

    if (!led) {
        std::cout << "Unable to open LED file\n";
        return;
    }

    LedHeader header;

    led.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.magic[0] != 'L' ||
        header.magic[1] != 'E' ||
        header.magic[2] != 'D' ||
        header.magic[3] != '1') {
        std::cout << "Invalid LED file\n";
        return;
    }

    for (uint32_t i = 0; i < header.fileCount; i++) {
        LedFileEntry entry;

        led.read(reinterpret_cast<char*>(&entry), sizeof(entry));

        if (targetName == entry.name) {
            std::vector<char> data(entry.size);

            led.seekg(entry.offset, std::ios::beg);

            if (entry.size > 0)
                led.read(data.data(), entry.size);

            std::cout << "\n";
            std::cout << "---------------------------------\n";

            for (char c : data)
                std::cout << c;

            std::cout << "\n---------------------------------\n\n";

            return;
        }
    }

    std::cout << "File \"" << targetName << "\" not found\n";
}

void ViewFiles(const std::string& ledPath) {
    std::ifstream led(ledPath, std::ios::binary);

    if (!led) {
        std::cout << "Unable to open LED file\n";
        return;
    }

    LedHeader header;

    led.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.magic[0] != 'L' ||
        header.magic[1] != 'E' ||
        header.magic[2] != 'D' ||
        header.magic[3] != '1') {
        std::cout << "Invalid LED file\n";
        return;
    }

    std::cout << "\n";
    std::cout << "Files:\n\n";

    if (header.fileCount == 0) {
        std::cout << "  <empty>\n\n";
        return;
    }

    for (uint32_t i = 0; i < header.fileCount; i++) {
        LedFileEntry entry{};

        led.read(reinterpret_cast<char*>(&entry), sizeof(entry));

        if (!led) {
            std::cout << "Unable to read file entry\n";
            return;
        }

        std::cout << "  " << entry.name << "  " << entry.size << " bytes\n";
    }

    std::cout << "\n";
}

void EditFile(const std::string& ledPath, const std::string& targetName) {

    bool justSaved = false;

    // Open LED file
    std::ifstream led(ledPath, std::ios::binary);

    if (!led) {
        std::cout << "Unable to open LED file\n";
        return;
    }

    // Read header
    LedHeader header{};

    led.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (!led) {
        std::cout << "Unable to read LED header\n";
        return;
    }

    if (header.magic[0] != 'L' ||
        header.magic[1] != 'E' ||
        header.magic[2] != 'D' ||
        header.magic[3] != '1') {
        std::cout << "Invalid LED file\n";
        return;
    }

    // Find target file
    LedFileEntry target{};
    bool found = false;

    for (uint32_t i = 0; i < header.fileCount; i++) {
        LedFileEntry entry{};

        led.read(reinterpret_cast<char*>(&entry), sizeof(entry));

        if (!led) {
            std::cout << "Unable to read file entry\n";
            return;
        }

        if (targetName == entry.name) {
            target = entry;
            found = true;
            break;
        }
    }

    if (!found) {
        std::cout << "File \"" << targetName << "\" not found\n";

        return;
    }

    // Read file data
    std::vector<char> data(target.size);

    led.clear();

    led.seekg(target.offset, std::ios::beg);

    if (!led) {
        std::cout << "Unable to find file data\n";
        return;
    }

    if (target.size > 0) {
        led.read(data.data(), target.size);

        if (!led) {
            std::cout << "Unable to read file data\n";
            return;
        }
    }

    led.close();

    // Convert data into lines
    std::vector<std::string> lines;

    std::string line;

    for (char c : data) {
        if (c == '\n') {
            lines.push_back(line);
            line.clear();
        }
        else if (c != '\r') {
            line += c;
        }
    }

    // Add final line
    if (!line.empty() || lines.empty()) {
        lines.push_back(line);
    }

    // Editor state
    size_t currentLine = 0;
    size_t currentColumn = 0;

    bool modified = false;
    bool editing = true;

    // Windows console
    HANDLE consoleInput = GetStdHandle(STD_INPUT_HANDLE);

    HANDLE consoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    // Disable normal line input
    DWORD inputMode;

    GetConsoleMode(consoleInput, &inputMode);

    DWORD originalInputMode = inputMode;

    inputMode &= ~ENABLE_LINE_INPUT;
    inputMode &= ~ENABLE_ECHO_INPUT;

    SetConsoleMode(consoleInput, inputMode);

    // Editor loop
    while (editing) {
        system("cls");

        // Calculate layout
        size_t lineNumberWidth = std::to_string(lines.size()).length();

        size_t textStartColumn = lineNumberWidth + 3;

        // Header
        std::cout << "-" << targetName << "----------------------------------\n\n";

        // Display lines
        for (size_t i = 0; i < lines.size(); i++) {
            std::cout << std::right;

            std::cout.width(static_cast<std::streamsize>(lineNumberWidth));

            std::cout << i + 1 << " | " << lines[i] << "\n";
        }

        std::cout << std::string(targetName.length(), '-') << "-----------------------------------\n\n";

        std::cout << "Ctrl+E Save    Ctrl+Q Exit";

        if (modified) {
            std::cout << "    Modified  ";
        }
        else if (justSaved) {
            std::cout << "    File saved";
        }

        std::cout << "\n";

        // Move cursor to current position
        COORD cursorPosition;

        cursorPosition.X = static_cast<SHORT>(textStartColumn + currentColumn);

        cursorPosition.Y = static_cast<SHORT>(2 + currentLine);

        SetConsoleCursorPosition(consoleOutput, cursorPosition);

        // Read keyboard event
        INPUT_RECORD record;
        DWORD eventsRead;

        ReadConsoleInput(consoleInput, &record, 1, &eventsRead);

        if (record.EventType != KEY_EVENT)
            continue;

        KEY_EVENT_RECORD& key = record.Event.KeyEvent;

        if (!key.bKeyDown)
            continue;

        bool ctrlPressed = (key.dwControlKeyState & LEFT_CTRL_PRESSED) || (key.dwControlKeyState & RIGHT_CTRL_PRESSED);

        // Ctrl+E
        if (ctrlPressed && key.wVirtualKeyCode == 'E') {
            std::string text;

            for (size_t i = 0; i < lines.size(); i++) {
                text += lines[i];

                if (i + 1 < lines.size())
                    text += '\n';
            }

            std::vector<char> newData(text.begin(), text.end());

            if (UpdateFile(ledPath, targetName, newData)) {
                modified = false;
                justSaved = true;
            }
            else {
                COORD messagePosition{ 0, 1 };

                SetConsoleCursorPosition(consoleOutput, messagePosition);

                std::cout << "Unable to save file               ";

                Sleep(1000);

                // Restore cursor to editing position
                COORD restorePosition;

                restorePosition.X = static_cast<SHORT>(textStartColumn + currentColumn);
                restorePosition.Y = static_cast<SHORT>(2 + currentLine);

                SetConsoleCursorPosition(consoleOutput, restorePosition);
            }

            continue;
        }


        // Ctrl+Q
        if (ctrlPressed && key.wVirtualKeyCode == 'Q') {
            if (modified) {
                system("cls");

                std::cout << "You have unsaved changes\n\n";

                std::cout << "Save before exiting? " << "[Y/N]\n";

                while (true) {
                    INPUT_RECORD saveRecord;
                    DWORD saveEvents;

                    ReadConsoleInput(consoleInput, &saveRecord, 1, &saveEvents);

                    if (saveRecord.EventType != KEY_EVENT)
                        continue;

                    KEY_EVENT_RECORD& saveKey = saveRecord.Event.KeyEvent;

                    if (!saveKey.bKeyDown)
                        continue;

                    if (saveKey.uChar.AsciiChar == 'Y' || saveKey.uChar.AsciiChar == 'y') {
                        std::string text;

                        for (size_t i = 0; i < lines.size(); i++) {
                            text += lines[i];

                            if (i + 1 < lines.size())
                                text += '\n';
                        }

                        std::vector<char> newData(text.begin(), text.end());

                        if (!UpdateFile(ledPath, targetName, newData)) {
                            std::cout << "\nUnable to save file";

                            Sleep(1000);

                            continue;
                        }

                        modified = false;
                        editing = false;

                        break;
                    }

                    if (saveKey.uChar.AsciiChar == 'N' || saveKey.uChar.AsciiChar == 'n') {
                        editing = false;
                        break;
                    }
                }
            }
            else {
                editing = false;
            }
            continue;
        }

        // ENTER
        if (key.wVirtualKeyCode == VK_RETURN) {
            std::string remainder = lines[currentLine].substr(currentColumn);

            lines[currentLine].erase(currentColumn);

            lines.insert(lines.begin() + currentLine + 1, remainder);

            currentLine++;
            currentColumn = 0;

            modified = true;
            continue;
        }

        // BACKSPACE
        if (key.wVirtualKeyCode == VK_BACK) {
            if (currentColumn > 0) {
                lines[currentLine].erase(currentColumn - 1, 1);

                currentColumn--;

                modified = true;
            }
            else if (currentLine > 0) {
                // Join with previous line

                currentColumn = lines[currentLine - 1].size();

                lines[currentLine - 1] += lines[currentLine];

                lines.erase(lines.begin() + currentLine);

                currentLine--;

                modified = true;
            }
            continue;
        }

        // LEFT
        if (key.wVirtualKeyCode == VK_LEFT) {
            if (currentColumn > 0) {
                currentColumn--;
            }
            else if (currentLine > 0) {
                currentLine--;

                currentColumn = lines[currentLine].size();
            }
            continue;
        }

        // RIGHT
        if (key.wVirtualKeyCode == VK_RIGHT)
        {
            if (currentColumn < lines[currentLine].size()) {
                currentColumn++;
            }
            else if (currentLine + 1 < lines.size()) {
                currentLine++;
                currentColumn = 0;
            }
            continue;
        }

        // UP
        if (key.wVirtualKeyCode == VK_UP) {
            if (currentLine > 0) {
                currentLine--;

                if (currentColumn > lines[currentLine].size()) {
                    currentColumn =
                        lines[currentLine].size();
                }
            }
            continue;
        }

        // DOWN
        if (key.wVirtualKeyCode == VK_DOWN) {
            if (currentLine + 1 < lines.size()) {
                currentLine++;

                if (currentColumn > lines[currentLine].size()) {
                    currentColumn = lines[currentLine].size();
                }
            }
            continue;
        }

        char character = key.uChar.AsciiChar;

        if (character >= 32 && character <= 126) {
            lines[currentLine].insert(currentColumn, 1, character);

            currentColumn++;

            modified = true;
        }
    }

    // Restore console input mode
    SetConsoleMode(consoleInput, originalInputMode);
    system("cls");
}