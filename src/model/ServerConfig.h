#pragma once
#include <QString>

struct ServerConfig {
    QString namedConfPath = "/etc/bind/named.conf";
    QString workDir       = "/etc/bind";
};
