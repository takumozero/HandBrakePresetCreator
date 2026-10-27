#pragma once
#include <QApplication>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QTemporaryDir>

class Theme {
    static QString icon(const QString &name,const QColor &color) {
        static QTemporaryDir directory;
        const QString path=directory.path()+"/"+name+"-"+color.name().mid(1)+".png";
        QImage image(36,36,QImage::Format_ARGB32_Premultiplied); image.fill(Qt::transparent);
        QPainter painter(&image); painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(color,3,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
        QPainterPath line;
        if(name=="check") { line.moveTo(8,18); line.lineTo(15,25); line.lineTo(28,11); }
        else if(name=="up") { line.moveTo(10,22); line.lineTo(18,14); line.lineTo(26,22); }
        else { line.moveTo(10,14); line.lineTo(18,22); line.lineTo(26,14); }
        painter.drawPath(line); painter.end(); image.save(path); return QDir::fromNativeSeparators(path);
    }
public:
    static bool dark() { return qApp->property("darkTheme").toBool(); }
    static void apply(bool dark,bool persist=false) {
        const QString background=dark?"#151c28":"#f4f6fa",surface=dark?"#1e2838":"#ffffff";
        const QString input=dark?"#243145":"#fcfdff",text=dark?"#e6edf8":"#243249";
        const QString muted=dark?"#a6b5cb":"#627189",border=dark?"#43536c":"#bdcadd";
        const QString soft=dark?"#28364c":"#eaf0fa",accent=dark?"#79a4ff":"#3868dc";
        const QString disabled=dark?"#78889f":"#7f8da2",alternate=dark?"#233044":"#f5f7fb";
        QPalette palette;
        for(auto group:{QPalette::Active,QPalette::Inactive,QPalette::Disabled}) {
            palette.setColor(group,QPalette::Window,QColor(background)); palette.setColor(group,QPalette::Base,QColor(surface));
            palette.setColor(group,QPalette::AlternateBase,QColor(alternate)); palette.setColor(group,QPalette::Button,QColor(input));
            for(auto role:{QPalette::Text,QPalette::WindowText,QPalette::ButtonText}) palette.setColor(group,role,QColor(group==QPalette::Disabled?disabled:text));
            palette.setColor(group,QPalette::Highlight,QColor(accent)); palette.setColor(group,QPalette::HighlightedText,QColor(dark?"#14223d":"#ffffff"));
            palette.setColor(group,QPalette::ToolTipBase,QColor(surface)); palette.setColor(group,QPalette::ToolTipText,QColor(text));
        }
        qApp->setPalette(palette); qApp->setProperty("darkTheme",dark);
        QString css=R"(
            QWidget { font-family: 'Segoe UI'; font-size: 14px; color: $text; }
            QMainWindow,QStatusBar { background: $background; }
            QLabel#title { font-size: 28px; font-weight: 700; }
            QLabel#muted,QLabel#section,QStatusBar { color: $muted; font-size: 13px; }
            QLabel#section { font-size: 11px; font-weight: 700; padding-bottom: 7px; }
            QPushButton { background: $surface; border: 1px solid $border; border-radius: 7px; padding: 10px 16px; font-weight: 600; }
            QPushButton:hover,QPushButton:checked { background: $soft; border-color: $accent; }
            QPushButton#primary { background: #3868dc; color: white; border-color: #3868dc; }
            QPushButton#primary:hover { background: #2858cc; }
            QPushButton:disabled,QPushButton#primary:disabled { background: $alternate; color: $disabled; border-color: $border; }
            QListWidget { background: $surface; border: 1px solid $border; border-radius: 9px; padding: 7px; outline: none; }
            QListWidget::item { padding: 13px 10px; border-radius: 6px; }
            QListWidget::item:selected,QTreeWidget::item:selected { background: $soft; color: $accent; }
            QTabWidget::pane { background: $surface; border: 1px solid $border; border-radius: 8px; top: 8px; padding: 10px; }
            QTabBar::tab { padding: 11px 17px; margin-right: 3px; background: $alternate; border: 1px solid transparent; border-top-left-radius: 7px; border-top-right-radius: 7px; }
            QTabBar::tab:selected { background: $surface; color: $accent; border-color: $border; }
            QScrollArea,QScrollArea > QWidget > QWidget { background: $surface; border: none; }
            QGroupBox { background: $surface; border: 1px solid $border; border-radius: 9px; margin-top: 17px; padding: 23px 16px 16px; font-weight: 600; }
            QGroupBox::title { subcontrol-origin: margin; left: 16px; padding: 0 6px; }
            QLineEdit,QPlainTextEdit,QComboBox,QDoubleSpinBox { background: $input; border: 1px solid $border; border-radius: 6px; padding: 8px; selection-background-color: #3868dc; selection-color: white; }
            QLineEdit:focus,QPlainTextEdit:focus,QComboBox:focus,QDoubleSpinBox:focus { border-color: $accent; }
            QLineEdit[invalid="true"] { border: 2px solid #d65c66; }
            QLineEdit:disabled,QComboBox:disabled,QDoubleSpinBox:disabled,QPlainTextEdit:disabled { background: $alternate; color: $disabled; }
            QComboBox { padding-right: 30px; }
            QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 28px; border: none; }
            QComboBox::down-arrow { image: url('$down'); width: 18px; height: 18px; }
            QComboBox QAbstractItemView { background: $surface; color: $text; border: 1px solid $border; selection-background-color: $soft; selection-color: $accent; padding: 5px; outline: none; }
            QDoubleSpinBox { padding-right: 28px; }
            QDoubleSpinBox::up-button,QDoubleSpinBox::down-button { width: 24px; border: none; background: transparent; }
            QDoubleSpinBox::up-button { subcontrol-origin: border; subcontrol-position: top right; }
            QDoubleSpinBox::down-button { subcontrol-origin: border; subcontrol-position: bottom right; }
            QDoubleSpinBox::up-arrow { image: url('$up'); width: 14px; height: 14px; }
            QDoubleSpinBox::down-arrow { image: url('$down'); width: 14px; height: 14px; }
            QCheckBox { spacing: 10px; padding: 3px 0; }
            QCheckBox:disabled { color: $disabled; }
            QCheckBox::indicator { width: 18px; height: 18px; border: 2px solid $border; border-radius: 4px; background: $input; }
            QCheckBox::indicator:hover { border-color: $accent; }
            QCheckBox::indicator:checked { background: #3868dc; border-color: #3868dc; image: url('$check'); }
            QCheckBox::indicator:disabled { border-color: $disabled; background: $alternate; }
            QCheckBox::indicator:checked:disabled { background: $disabled; image: url('$check'); }
            QScrollBar:vertical { background: $surface; width: 12px; margin: 2px; }
            QScrollBar::handle:vertical { background: $border; border-radius: 4px; min-height: 32px; }
            QScrollBar:horizontal { background: $surface; height: 12px; margin: 2px; }
            QScrollBar::handle:horizontal { background: $border; border-radius: 4px; min-width: 32px; }
            QScrollBar::handle:hover { background: $muted; }
            QScrollBar::add-line,QScrollBar::sub-line { width: 0; height: 0; border: none; }
            QScrollBar::add-page,QScrollBar::sub-page { background: transparent; }
            QAbstractScrollArea::corner { background: $surface; border: none; }
            QSplitter::handle { background: $background; }
            QPlainTextEdit#preview { font-family: 'Consolas'; font-size: 12px; border: none; padding: 15px; }
            QTreeWidget { background: $surface; alternate-background-color: $alternate; border: none; outline: none; }
            QTreeWidget::item { padding: 6px 5px; }
            QHeaderView::section { background: $alternate; color: $muted; padding: 9px; border: none; }
            QMenu,QToolTip { background: $surface; color: $text; border: 1px solid $border; padding: 6px; }
            QMenu::item { padding: 8px 18px; border-radius: 5px; }
            QMenu::item:selected { background: $soft; }
        )";
        for(const auto &entry:QList<QPair<QString,QString>>{{"background",background},{"surface",surface},{"input",input},{"text",text},{"muted",muted},{"border",border},{"soft",soft},{"accent",accent},{"disabled",disabled},{"alternate",alternate},{"down",icon("down",QColor(muted))},{"up",icon("up",QColor(muted))},{"check",icon("check",Qt::white)}})
            css.replace("$"+entry.first,entry.second);
        qApp->setStyleSheet(css);
        if(persist) QSettings("HandbrakePresetCreator","PresetStudio").setValue("appearance/dark",dark);
    }
};
