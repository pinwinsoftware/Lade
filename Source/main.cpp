#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>

#include "Editor.h"
#include "LedFile.h"

using namespace std;

int main() {
    std::cout << "=================================\n";
    std::cout << "     Lite Engine Data Editor     \n";
    std::cout << "=================================\n\n";

    while (true) {
        std::cout << "LED>";

        std::string command;
        std::cin >> command;

        if (command == "HELP") {
            std::cout << "\n";
            std::cout << "Commands:\n\n";
            std::cout << "  HELP    Shows this help\n";
            std::cout << "  OPEN    Opens a led file\n";
            std::cout << "  NEW     Creates a new led file\n";
            std::cout << "  INFO    Shows information about Lade\n";
            std::cout << "  EXIT    Exits the Lade\n\n";
        }
        else if (command == "INFO") {
            std::cout << "\n";
            std::cout << "  Lade version 1.0\n";
            std::cout << "  Developed by Larion Naumenko\n";
            std::cout << "  Copyright (C) Pinwin Software\n\n";
        }
        else if (command == "NEW") {
            std::cout << "Name?\n";

            std::string name;
            std::cin >> name;

            std::string ledPath = "LED/" + name + ".LED";

            std::ofstream file(ledPath, std::ios::binary);

            if (!file) {
                std::cout << "Unable to create file\n";
            }
            else {
                system("cls");

                LedHeader header;

                header.magic[0] = 'L';
                header.magic[1] = 'E';
                header.magic[2] = 'D';
                header.magic[3] = '1';
                header.fileCount = 0;

                file.write(reinterpret_cast<const char*>(&header), sizeof(header));

                // IMPORTANT:
                // Close the file BEFORE opening it again in Editor().
                file.close();

                std::cout << "Created: " << ledPath  << "\n";

                Editor(ledPath);
            }
        }
        else if (command == "OPEN") {
            std::cout << "File?\n";

            std::string path;
            std::cin >> path;

            std::ifstream file(path, std::ios::binary);

            if (!file) {
                std::cout << "Unable to open file\n";
                continue;
            }

            LedHeader header{};

            file.read(reinterpret_cast<char*>(&header), sizeof(header));

            file.close();

            // Check LED signature
            if (header.magic[0] != 'L' ||
                header.magic[1] != 'E' ||
                header.magic[2] != 'D' ||
                header.magic[3] != '1') {
                std::cout << "Invalid LED file\n";
                continue;
            }

            system("cls");
            std::cout << "Opened: " << path << "\n";
            Editor(path);
        }
        else if (command == "VIEW") {
            std::filesystem::path ledDirectory = "LED";

            if (!std::filesystem::exists(ledDirectory)) {
                std::cout << "LED directory does not exist\n";
                continue;
            }

            std::cout << "\n";
            std::cout << "LED files:\n\n";

            bool found = false;

            for (const auto& entry : std::filesystem::directory_iterator(ledDirectory)) {
                if (!entry.is_regular_file())
                    continue;

                if (entry.path().extension() != ".LED")
                    continue;

                found = true;

                std::cout << "  "
                    << entry.path().filename().string()
                    << "\n";
            }

            if (!found) {
                std::cout << "  <no LED files>\n";
            }

            std::cout << "\n";
        }
        else if (command == "EXIT") {
            return 0;
        }
        else {
            std::cout << "Unknown command \"" << command << "\"\n";
        }
    }
	return 0;
}