#include "DocumentTests.h"
#include "EditorTests.h"
#include <QApplication>
#include <QStyleFactory>
#include <cstdio>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("HandBrake Preset Creator Tests");
    app.setStyle(QStyleFactory::create("Fusion"));
    Theme::apply(false);
    qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &message) {
        const auto bytes = message.toUtf8();
        std::fprintf(stderr, "%s\n", bytes.constData());
    });

    const auto arguments = app.arguments();
    if (arguments.contains("--self-test"))
        return selfTest();
    if (arguments.contains("--delete-test"))
        return deletionTest();

    QString library;
    for (int i = 1; i < arguments.size(); ++i)
        if (!arguments[i].startsWith("--")) {
            library = arguments[i];
            break;
        }
    if (library.isEmpty()) {
        std::fprintf(stderr, "Usage: HandBrakePresetCreator.Tests.exe [presets.json] --self-test|--delete-test|--editor-test|--easy-test|--ui-rules-test|--delete-ui-test\n");
        return 2;
    }

    EditorTests window;
    if (!window.loadFile(library))
        return 2;
    if (arguments.contains("--editor-test"))
        return window.testEditing();
    if (arguments.contains("--easy-test"))
        return window.testEasyView();
    if (arguments.contains("--ui-rules-test"))
        return window.testRequestedFeatures();
    if (arguments.contains("--delete-ui-test"))
        return window.testDeletion();
    std::fprintf(stderr, "Specify one supported test switch.\n");
    return 2;
}
