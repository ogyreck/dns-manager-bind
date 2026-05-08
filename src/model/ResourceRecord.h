#pragma once
#include <cstdint>
#include <QString>

enum class RecordType { A, AAAA, NS, MX, CNAME, PTR, SOA, TXT };

struct ResourceRecord {
    QString name;
    RecordType type;
    uint32_t ttl = 0;
    QString data;
    int priority = 0;
};
