#pragma once

#include <string>
#include <map>

class Settings {
public:
    Settings();

    void Load();
    void Dump();

    bool Has(const std::string& setting);
    std::string Get(const std::string& setting);
    void Set(const std::string& setting, std::string value);

private:
    const std::string _file = "./dshba.ini";
    std::map<std::string, std::string> _settings;
};
