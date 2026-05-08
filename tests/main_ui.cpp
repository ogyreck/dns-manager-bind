#include <gtest/gtest.h>
#include <QApplication>

int main(int argc, char **argv) {
    // offscreen — чтобы диалоги работали без дисплея в CI
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
