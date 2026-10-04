#include <iostream>
#include <string>
#include <filesystem>

#include "LedFile.h"

void Editor(const std::string& ledPath) {
    while (true) {
        std::cout << "LED>";

        std::string command;
        std::cin >> command;

        if (command == "HELP") {
            std::cout << "\n";
            std::cout << "Commands:\n\n";
            std::cout << "  INCLUDE  Include existing file\n";
            std::cout << "  EXPORT   Export asset from led\n";
            std::cout << "  CREATE   Create new file inside of led\n";
            std::cout << "  DELETE   Remove asset from led\n";
            std::cout << "  BINARY   View a file as binary\n";
            std::cout << "  TEXT     View a file as text\n";
            std::cout << "  VIEW     View existing files\n";
            std::cout << "  EDIT     Edit existing file\n";
            std::cout << "  EXIT     Finish editing\n\n";
        }

        else if (command == "INCLUDE") {
            std::cout << "File?\n";

            std::string sourcePath;

            std::getline(std::cin >> std::ws, sourcePath);

            if (IncludeFile(ledPath, sourcePath)) {
                std::cout << "File included.\n";
            }
            else {
                std::cout << "Unable to include file.\n";
            }
        }

        else if (command == "EXPORT") {
            std::cout << "Target?\n";

            std::string fileName;
            std::cin >> fileName;

            std::cout << "Output?\n";

            std::string outputPath;
            std::cin >> outputPath;

            std::filesystem::path destination(outputPath);

            if (std::filesystem::is_directory(destination)) {
                destination /= fileName;
            }

            if (ExportFile(ledPath, fileName, destination.string())) {
                std::cout << "File exported to "
                    << destination.string()
                    << "\n";
            }
            else {
                std::cout << "Unable to export file.\n";
            }
        }

        else if (command == "CREATE")
        {
            std::cout << "Type?\n";

            std::string type;
            std::cin >> type;

            std::cout << "Name?\n";

            std::string name;
            std::cin >> name;

            std::string fileName = name + "." + type;

            if (CreateNewFile(ledPath, fileName)) {
                std::cout << "Created \"" << fileName << "\".\n";
            }
            else {
                std::cout << "Unable to create file.\n";
            }
        }

        else if (command == "DELETE") {
            std::cout << "Target?\n";

            std::string targetName;
            std::cin >> targetName;

            if (DeleteAssetFile(ledPath, targetName)) {
                std::cout << "File deleted.\n";
            }
            else {
                std::cout << "Unable to delete file.\n";
            }
        }

        else if (command == "BINARY") {
            std::cout << "Target?\n";

            std::string targetName;

            std::getline(std::cin >> std::ws, targetName);

            ViewBinary(ledPath, targetName);
        }

        else if (command == "TEXT") {
            std::cout << "Target?\n";

            std::string targetName;
            std::cin >> targetName;

            ViewTextFile(ledPath, targetName);
        }

        else if (command == "VIEW") {
            ViewFiles(ledPath);
        }

        else if (command == "EDIT") {
            std::cout << "Target?\n";

            std::string targetName;
            std::cin >> targetName;

            EditFile(ledPath, targetName);
        }

        else if (command == "EXIT") {
            system("cls");
            return;
        }

        else {
            std::cout << "Unknown command \"" << command << "\"\n";
        }
    }
}