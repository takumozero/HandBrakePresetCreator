#include "editor.h"
#include "theme.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("HandBrake Preset Creator");
    app.setStyle(QStyleFactory::create("Fusion"));

    QCommandLineParser parser;
    parser.setApplicationDescription("Create and edit HandBrake preset JSON libraries.");
    parser.addHelpOption();
    parser.addPositionalArgument("preset", "Optional preset JSON file to open.", "[preset.json]");
    parser.process(app);

    Theme::apply(QSettings("HandbrakePresetCreator", "PresetStudio")
                     .value("appearance/dark", false).toBool());
    Editor window;
    const auto files = parser.positionalArguments();
    if (!files.isEmpty())
        window.loadFile(files.first());
    window.show();
    return app.exec();
}
