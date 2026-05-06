#include <gtest/gtest.h>
#include <QProcess>
#include <QString>
#include "dns/ConfigValidator.h"

static QString fixturesDir() {
    return QString(FIXTURES_DIR);
}

// Проверяем, доступен ли named-checkconf
static bool isNamedCheckconfAvailable() {
    QProcess proc;
    proc.start("which", {"named-checkconf"});
    proc.waitForFinished(3000);
    return proc.exitCode() == 0;
}

TEST(ConfigValidator, CheckConfValid) {
    if (!isNamedCheckconfAvailable())
        GTEST_SKIP() << "named-checkconf не установлен";

    ConfigValidator validator;
    QString error;
    bool ok = validator.checkConf(fixturesDir() + "/named_simple.conf", &error);

    EXPECT_TRUE(ok) << "Ожидался успех для корректного named.conf: " << error.toStdString();
    EXPECT_TRUE(error.isEmpty()) << "error должен быть пуст при успехе";
}

TEST(ConfigValidator, CheckConfBroken) {
    if (!isNamedCheckconfAvailable())
        GTEST_SKIP() << "named-checkconf не установлен";

    ConfigValidator validator;
    QString error;
    bool ok = validator.checkConf(fixturesDir() + "/named_broken.conf", &error);

    EXPECT_FALSE(ok)           << "Ожидалась ошибка для сломанного named.conf";
    EXPECT_FALSE(error.isEmpty()) << "error должен содержать описание ошибки";
}
