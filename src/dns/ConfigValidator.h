#pragma once
#include <QString>

// Валидирует конфигурацию BIND9 через named-checkconf и named-checkzone.
// Все внешние команды запускаются через QProcess.
class ConfigValidator {
public:
    // Запускает named-checkconf <namedConfPath>.
    // Возвращает true при exitCode == 0; при ошибке записывает stderr в *error.
    bool checkConf(const QString &namedConfPath, QString *error = nullptr);

    // Запускает named-checkzone -z <zoneName> <zoneFilePath>.
    // Возвращает true при exitCode == 0; при ошибке записывает stderr в *error.
    bool checkZone(const QString &zoneName, const QString &zoneFilePath, QString *error = nullptr);
};
