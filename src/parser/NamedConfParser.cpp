#include "parser/NamedConfParser.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

/**
 * @brief Главная точка входа для парсинга конфигурации
 * 
 * Очищает состояние парсера и запускает рекурсивный парсинг
 */
QList<Zone> NamedConfParser::parse(const QString &filePath) {
    m_parsedFiles.clear();      // Очищаем кэш обработанных файлов
    m_lastError.clear();        // Очищаем текст ошибки
    return doParse(filePath);   // Начинаем рекурсивный разбор
}

/**
 * @brief Основной рекурсивный метод парсинга файла конфигурации
 * 
 * Алгоритм:
 * 1. Проверка существования файла и циклических включений
 * 2. Удаление комментариев из содержимого
 * 3. Обработка include-директив (рекурсивный вызов)
 * 4. Извлечение информации о зонах с помощью regex
 */
QList<Zone> NamedConfParser::doParse(const QString &filePath) {
    QList<Zone> zones;

    // === ЭТАП 1: Проверка существования файла ===
    const QString canonical = QFileInfo(filePath).canonicalFilePath();
    if (canonical.isEmpty()) {
        m_lastError = "Файл не найден: " + filePath;
        qWarning() << "[NamedConfParser] Ошибка открытия:" << filePath;
        return zones;
    }

    // === ЭТАП 2: Защита от циклических включений ===
    // Если файл уже был обработан, пропускаем его
    if (m_parsedFiles.contains(canonical)) {
        qWarning() << "[NamedConfParser] Циклический include:" << filePath;
        return zones;
    }
    m_parsedFiles.insert(canonical);  // Добавляем в кэш

    qDebug() << "[NamedConfParser] Парсим файл:" << filePath;

    // === ЭТАП 3: Чтение содержимого файла ===
    QFile file(canonical);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = "Не удалось открыть файл: " + filePath;
        qWarning() << "[NamedConfParser] Ошибка открытия:" << filePath;
        return zones;
    }

    QString content = QTextStream(&file).readAll();
    file.close();

    // === ЭТАП 4: Удаление комментариев ===
    // Удаляем блочные комментарии /* ... */
    content.remove(QRegularExpression("/\\*.*?\\*/", QRegularExpression::DotMatchesEverythingOption));
    // Удаляем строчные комментарии //...
    content.remove(QRegularExpression("//[^\n]*"));
    // Удаляем комментарии в стиле shell #...
    content.remove(QRegularExpression("#[^\n]*"));

    // Получаем директорию файла для разрешения относительных путей
    const QString currentDir = QFileInfo(canonical).absolutePath();

    // === ЭТАП 5: Обработка include-директив ===
    // Ищем все строки вида: include "path/to/file.conf";
    QRegularExpression includeRe("include\\s+\"([^\"]+)\"\\s*;");
    auto incIt = includeRe.globalMatch(content);
    while (incIt.hasNext()) {
        auto incMatch = incIt.next();
        QString includePath = incMatch.captured(1);  // Извлекаем путь из include
        
        // Если путь относительный, разрешаем его относительно текущей директории
        if (!QFileInfo(includePath).isAbsolute())
            includePath = currentDir + "/" + includePath;

        qDebug() << "[NamedConfParser] include→" << includePath;
        
        // Рекурсивно парсим включаемый файл и добавляем найденные зоны
        zones.append(doParse(includePath));
    }

    // === ЭТАП 6: Извлечение информации о зонах ===
    // Регулярное выражение для поиска определений зон
    // Поддерживает одноуровневое вложение {} для зон (covers most common cases):
    // zone "example.com" IN { ... }
    // zone "example.com" { ... }
    QRegularExpression zoneRe(
        "zone\\s+\"([^\"]+)\"\\s*(?:IN\\s*)?\\{([^{}]*(?:\\{[^{}]*\\}[^{}]*)*)\\}",
        QRegularExpression::CaseInsensitiveOption);

    auto it = zoneRe.globalMatch(content);
    while (it.hasNext()) {
        auto match = it.next();
        Zone zone;
        zone.name = match.captured(1);                // Имя зоны (например, "example.com")
        const QString body = match.captured(2);       // Содержимое блока зоны

        // === Определение типа зоны ===
        QRegularExpression typeRe("type\\s+(\\w+)\\s*;");
        auto typeMatch = typeRe.match(body);
        if (!typeMatch.hasMatch())
            continue;  // Пропускаем, если тип не найден

        const QString t = typeMatch.captured(1).toLower();
        if (t == "master")
            zone.type = ZoneType::Master;
        else if (t == "slave")
            zone.type = ZoneType::Slave;
        else
            continue;  // Неподдерживаемый тип, пропускаем

        // === Поиск пути к файлу зоны ===
        // Ищем строку: file "path/to/zone/db.example.com";
        QRegularExpression fileRe("file\\s+\"([^\"]+)\"\\s*;");
        auto fileMatch = fileRe.match(body);
        if (fileMatch.hasMatch()) {
            QString zonePath = fileMatch.captured(1);
            // Разрешаем относительные пути
            if (!QFileInfo(zonePath).isAbsolute())
                zonePath = currentDir + "/" + zonePath;
            zone.filePath = zonePath;
        }

        // === Поиск Master-сервера для Slave-зон ===
        // Ищем строку вида: masters { 192.168.1.1; };
        QRegularExpression mastersRe("masters\\s*\\{\\s*([\\d.]+)");
        auto mastersMatch = mastersRe.match(body);
        if (mastersMatch.hasMatch())
            zone.masterIp = mastersMatch.captured(1);

        // === Определение типа зоны (Forward/Reverse) ===
        // Reverse-зоны содержат "in-addr.arpa" или "ip6.arpa" в имени
        if (zone.name.contains("in-addr.arpa", Qt::CaseInsensitive) ||
            zone.name.contains("ip6.arpa", Qt::CaseInsensitive))
            zone.view = ZoneView::Reverse;
        else
            zone.view = ZoneView::Forward;

        // Логирование найденной зоны
        qDebug() << "[NamedConfParser] Зона:" << zone.name
                 << "тип:" << (zone.type == ZoneType::Master ? "master" : "slave")
                 << "вид:" << (zone.view == ZoneView::Forward ? "forward" : "reverse");

        zones.append(zone);
    }

    return zones;
}
