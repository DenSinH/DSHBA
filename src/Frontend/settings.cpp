#include "settings.h"

#include <sstream>
#include <fstream>
#include <string>
#include <regex>
#include <utility>

Settings::Settings() {
    std::ifstream file(_file);
    if (file.good()) {
        Load();
        file.close();
    }
    else {
        Dump();
    }
}

void Settings::Dump() {
    std::ofstream file;
    file.open(_file, std::ofstream::out | std::ofstream::trunc);

    if (file.is_open()) {
        for (auto const& [key, val] : _settings) {
            file << "\"" << key << "\" = \"" << val << "\"" << std::endl;
        }

        file.close();
    }
}

void Settings::Load() {
    std::string line;
    std::ifstream file(_file);

    if (file.fail()) {
        printf("Could not open settings file\n");
        return;
    }

    _settings.clear();
    const std::regex setting(R"(^\s*"(\w+)\"\s*=\s*"(.+)\"\s*$)");

    std::smatch match;

    while (std::getline(file, line)) {
        if (std::regex_match(line, match, setting)) {
            _settings[match[1].str()] = match[2].str();
        }
    }
}

bool Settings::Has(const std::string& setting) {
    return _settings.contains(setting);
}

std::string Settings::Get(const std::string& setting) {
    if (_settings.contains(setting)) {
        return _settings[setting];
    }
    return nullptr;
}

void Settings::Set(const std::string& setting, std::string value) {
    _settings[setting] = std::move(value);
}