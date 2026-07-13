#pragma once

#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include "types.h"
#include "config/settings.h"

class ConfigManager {
public:
    ConfigManager();

    bool begin();
    bool load();
    bool save();

    void resetToDefaults();

    SystemConfig& config();
    const SystemConfig& config() const;

    uint32_t incubationDays() const;
    void setIncubationDays(uint32_t days);

    bool isDirty() const;
    bool saveIfDirty();

private:
    static constexpr uint32_t CONFIG_VERSION = 1;
    static constexpr const char* TEMP_FILE  = "/config.tmp";
    static constexpr const char* BACKUP_FILE = "/config.bak";

    SystemConfig m_config;
    uint32_t m_incubationDays = 0;
    bool m_dirty = false;

    bool writeToFile(const char* path);
    bool readFromFile(const char* path, JsonDocument& doc);
    void validateAndClamp();
    bool migrateConfig(JsonDocument& doc, uint32_t fileVersion);
};
