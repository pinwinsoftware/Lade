#pragma once
#include <string>
#include <cstdint>
#include <vector>

struct LedHeader {
    char magic[4];
    uint32_t fileCount;
};

struct LedFileEntry {
    char name[64];
    uint32_t offset;
    uint32_t size;
};

bool UpdateFile(const std::string& ledPath, const std::string& fileName, const std::vector<char>& newData);

bool IncludeFile(const std::string& ledPath, const std::string& sourcePath);

bool ExportFile(const std::string& ledPath, const std::string& fileName, const std::string& outputPath);

bool CreateNewFile(const std::string& ledPath, const std::string& fileName);

bool DeleteAssetFile(const std::string& ledPath, const std::string& targetName);

void ViewBinary(const std::string& ledPath, const std::string& targetName);

void ViewTextFile(const std::string& ledPath, const std::string& targetName);

void ViewFiles(const std::string& ledPath);

void EditFile(const std::string& ledPath, const std::string& targetName);