#include "parser/ZoneFileParser.h"
#include <QDebug>
#include <QFile>
#include <QStringList>
#include <QTextStream>

static RecordType recordTypeFromString(const QString &s) {
    if (s == "A") return RecordType::A;
    if (s == "AAAA") return RecordType::AAAA;
    if (s == "NS") return RecordType::NS;
    if (s == "MX") return RecordType::MX;
    if (s == "CNAME") return RecordType::CNAME;
    if (s == "PTR") return RecordType::PTR;
    if (s == "SOA") return RecordType::SOA;
    if (s == "TXT") return RecordType::TXT;
    return static_cast<RecordType>(-1);
}


//проверяет, является ли строка известным типом записи
static bool isKnownType(const QString &s) {
    static const QStringList kTypes = {"A", "AAAA", "NS", "MX", "CNAME", "PTR", "SOA", "TXT"};
    return kTypes.contains(s);
}

QList<ResourceRecord> ZoneFileParser::parse(const QString &filePath) {
    m_lastError.clear();
    QList<ResourceRecord> records;

    // Открываем файл
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = "Не удалось открыть файл: " + filePath;
        qWarning() << "[ZoneFileParser] Ошибка открытия:" << filePath;
        return records;
    }

    qDebug() << "[ZoneFileParser] Парсим:" << filePath;

    // Читаем все сторки 
    QTextStream in(&file);
    QStringList rawLines;
    while (!in.atEnd())
        rawLines.append(in.readLine());
    file.close();


    QStringList lines;
    bool inParens = false;
    QString accumulated;
    for (const QString &rawLine : rawLines) {
        // Обрезаем комментарий ';' (но не внутри кавычек TXT)
        int semicolon = -1;
        bool inQuote = false;
        for (int i = 0; i < rawLine.size(); ++i) {
            if (rawLine[i] == '"') inQuote = !inQuote;
            if (!inQuote && rawLine[i] == ';') { semicolon = i; break; }
        }
        // Обрезаем комментарий и убираем пробелы по краям
        QString line = (semicolon >= 0 ? rawLine.left(semicolon) : rawLine).trimmed();
        if (line.isEmpty()) continue;

        // Проверяем, находимся ли внутри многострочной записи
        if (inParens) {
            accumulated += " " + line;
            if (line.contains(')')) {
                // Убираем скобки и лишние пробелы
                accumulated.remove('(').remove(')');
                lines.append(accumulated.simplified());
                accumulated.clear();
                inParens = false;
            }
        } else {
            // Если строка начинает многострочную запись
            if (line.contains('(') && !line.contains(')')) {
                accumulated = line;
                accumulated.remove('(');
                inParens = true;
            } else {
                // Обычная однострочная запись
                QString l = line;
                l.remove('(').remove(')');
                lines.append(l.simplified());
            }
        }
    }

    uint32_t defaultTtl = 0;
    QString lastOwner;

    for (const QString &line : lines) {
        // $TTL директива
        if (line.startsWith("$TTL", Qt::CaseInsensitive)) {
            QStringList parts = line.split(' ', QString::SkipEmptyParts);
            if (parts.size() >= 2)
                defaultTtl = parts[1].toUInt();
            continue;
        }
       
        if (line.startsWith('$'))
            continue;

        QStringList tokens = line.split(' ', QString::SkipEmptyParts);
        if (tokens.isEmpty())
            continue;

        // Разбираем: [name] [ttl] [class] type data...
        int idx = 0;
        QString owner;


        bool startsWithWhitespace = false;
        {
            const QString t0 = tokens[0].toUpper();
            if (isKnownType(t0)) {
                startsWithWhitespace = true;
            } else if ((t0 == "IN" || t0 == "CH" || t0 == "HS") &&
                       tokens.size() > 1 && isKnownType(tokens[1].toUpper())) {
                // строка вида «IN NS ...» без owner — старый формат без leading whitespace
                startsWithWhitespace = true;
            }
        }

        if (startsWithWhitespace) {
            owner = lastOwner;
        } else {
            owner = tokens[idx++];
        }

        if (idx >= tokens.size())
            continue;

        // Опциональный TTL (число)
        uint32_t ttl = defaultTtl;
        bool isTtl = false;
        tokens[idx].toUInt(&isTtl);
        if (isTtl) {
            ttl = tokens[idx++].toUInt();
        }

        if (idx >= tokens.size())
            continue;

        // Опциональный класс IN/CH/HS
        if (tokens[idx].compare("IN", Qt::CaseInsensitive) == 0 ||
            tokens[idx].compare("CH", Qt::CaseInsensitive) == 0 ||
            tokens[idx].compare("HS", Qt::CaseInsensitive) == 0) {
            idx++;
        }

        if (idx >= tokens.size())
            continue;

        const QString typeStr = tokens[idx++].toUpper();
        if (!isKnownType(typeStr)) {
            qWarning() << "[ZoneFileParser] Неизвестный тип:" << typeStr;
            continue;
        }

        RecordType rtype = recordTypeFromString(typeStr);
        ResourceRecord rr;
        rr.name = owner;
        rr.type = rtype;
        rr.ttl = ttl;

        if (rtype == RecordType::MX) {
            // priority name
            if (idx < tokens.size()) rr.priority = tokens[idx++].toInt();
            if (idx < tokens.size()) rr.data = tokens[idx++];
        } else if (rtype == RecordType::SOA) {
            // ns admin serial refresh retry expire minimum
            QStringList soaFields = tokens.mid(idx);
            rr.data = soaFields.join(" ");
        } else {
            if (idx < tokens.size())
                rr.data = tokens.mid(idx).join(" ");
        }

        lastOwner = owner;

        qDebug() << "[ZoneFileParser] Запись:" << rr.name << typeStr << rr.data;
        records.append(rr);
    }

    return records;
}
