#pragma once
#include <QString>
#include "model/Zone.h"

class ConfigValidator;

// Атомарно записывает файл зоны: .tmp → валидация → rename.
// Живёт в parser/ и не знает о QProcess — validator передаётся снаружи.
class ConfigWriter {
public:
    // Формирует текст зоны, записывает в destPath+".tmp",
    // валидирует через validator (если не nullptr),
    // атомарно переименовывает в destPath.
    // Возвращает false при любой ошибке; *error содержит описание.
    bool writeZoneFile(const Zone &zone,
                       const QString &destPath,
                       ConfigValidator *validator,
                       QString *error = nullptr);
};
