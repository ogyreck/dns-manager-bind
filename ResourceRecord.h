#ifndef RESOURCERECORD_H
#define RESOURCERECORD_H

#pragma once
#include <QString>

enum class RecordType { A, AAAA, NS, MX, CNAME, PTR, SOA, TXT };

struct ResourceRecord {
    QString name;        // "www", "@", "mail"
    RecordType type;     // A, MX и тд
    uint32_t ttl;        // тайм ту лив
    QString data;        // "192.168.1.1"
    int priority = 0;    // только для MX
};


#endif // RESOURCERECORD_H
