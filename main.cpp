#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("HandBrake Preset Creator");

    QMainWindow window;
    window.setWindowTitle(app.applicationName());
    window.resize(900, 600);

    auto *central = new QWidget(&window);
    auto *layout = new QVBoxLayout(central);
    auto *heading = new QLabel("HandBrake Preset Creator", central);
    auto headingFont = heading->font();
    headingFont.setPointSize(22);
    headingFont.setBold(true);
    heading->setFont(headingFont);
    layout->addWidget(heading);

    auto *intro = new QLabel(
        "Create and edit HandBrake presets.\n"
        "The Qt application is ready. Preset controls will be added next.", central);
    intro->setWordWrap(true);
    layout->addWidget(intro);
    layout->addStretch();

    auto *closeButton = new QPushButton("Close", central);
    QObject::connect(closeButton, &QPushButton::clicked, &window, &QWidget::close);
    layout->addWidget(closeButton, 0, Qt::AlignRight);
    layout->setContentsMargins(32, 32, 32, 32);
    window.setCentralWidget(central);
    window.statusBar()->showMessage("Ready");
    window.show();

    return app.exec();
}
