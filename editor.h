#pragma once
#include "presetdocument.h"
#include "easyview.h"
#include "theme.h"
#include <QCloseEvent>
#include <QComboBox>
#include <QDoubleValidator>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QSplitter>
#include <QStatusBar>
#include <QStyledItemDelegate>
#include <QTabWidget>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <cmath>

class SettingDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QWidget *createEditor(QWidget *parent,const QStyleOptionViewItem &,const QModelIndex &index) const override {
        if(index.column()!=1) return nullptr;
        const int type=index.data(Qt::UserRole).toInt();
        const auto options=index.data(Qt::UserRole+2).toStringList();
        if(type==QJsonValue::Bool || (type==QJsonValue::String && !options.isEmpty())) {
            auto *combo=new QComboBox(parent); combo->setEditable(type!=QJsonValue::Bool);
            combo->addItems(type==QJsonValue::Bool?QStringList{"false","true"}:options); return combo;
        }
        auto *line=new QLineEdit(parent);
        if(type==QJsonValue::Double) {
            auto *validator=new QDoubleValidator(line); validator->setLocale(QLocale::c()); line->setValidator(validator);
        }
        return line;
    }
    void setEditorData(QWidget *editor,const QModelIndex &index) const override {
        if(auto *combo=qobject_cast<QComboBox *>(editor)) combo->setCurrentText(index.data().toString());
        else if(auto *line=qobject_cast<QLineEdit *>(editor)) line->setText(index.data().toString());
    }
    void setModelData(QWidget *editor,QAbstractItemModel *model,const QModelIndex &index) const override {
        QString text;
        if(auto *combo=qobject_cast<QComboBox *>(editor)) text=combo->currentText();
        else if(auto *line=qobject_cast<QLineEdit *>(editor)) { if(!line->hasAcceptableInput()) return; text=line->text(); }
        model->setData(index,text,Qt::EditRole);
    }
};

class Editor : public QMainWindow {
    friend class EditorTests;
    PresetDocument document;
    QJsonObject selected;
    int selectedIndex=-1;
    QString path;
    bool dirty=false,loading=false,selectedDirty=false,previewStale=true;
    QListWidget *list;
    QTreeWidget *settings;
    QLineEdit *name,*search;
    QPlainTextEdit *preview;
    QTabWidget *tabs;
    EasyView *easy;
    QLabel *source;
    QPushButton *saveButton,*saveAsButton,*deleteButton;
    QTimer previewTimer;
    QMap<QString,QStringList> suggestions;
    QMap<QString,QJsonValue> examples;
    static constexpr int PathRole=Qt::UserRole+1;
public:
    Editor() {
        setMinimumSize(850,650); resize(1220,840);
        auto *root=new QWidget; auto *layout=new QVBoxLayout(root);
        layout->setContentsMargins(28,24,28,20); layout->setSpacing(16);
        auto *top=new QHBoxLayout; auto *title=new QLabel("Preset Studio"); title->setObjectName("title");
        top->addWidget(title); top->addStretch();
        auto *darkMode=new QPushButton("Dark mode"); darkMode->setObjectName("themeToggle"); darkMode->setCheckable(true); darkMode->setChecked(Theme::dark()); top->addWidget(darkMode);
        connect(darkMode,&QPushButton::toggled,this,[](bool dark){ Theme::apply(dark,true); });
        auto *open=new QPushButton("Open presets…"); top->addWidget(open);
        auto *create=new QPushButton("New preset"); top->addWidget(create); auto *menu=new QMenu(create);
        auto *addHere=menu->addAction("Add to current JSON file"); auto *newFile=menu->addAction("Create in a new JSON file…"); create->setMenu(menu);
        saveAsButton=new QPushButton("Save as…"); top->addWidget(saveAsButton);
        saveButton=new QPushButton("Save"); saveButton->setObjectName("primary"); top->addWidget(saveButton); layout->addLayout(top);
        source=new QLabel("Open a HandBrake JSON library to edit its presets."); source->setObjectName("muted"); source->setWordWrap(true); layout->addWidget(source);
        auto *split=new QSplitter; auto *sidebar=new QWidget; auto *side=new QVBoxLayout(sidebar); side->setContentsMargins(0,0,12,0);
        auto *caption=new QLabel("PRESET LIBRARY"); caption->setObjectName("section"); side->addWidget(caption);
        list=new QListWidget; side->addWidget(list);
        deleteButton=new QPushButton("Delete preset…"); deleteButton->setObjectName("deletePreset"); side->addWidget(deleteButton);
        auto *deleteAction=new QAction("Delete preset…",list); deleteAction->setShortcut(QKeySequence::Delete); deleteAction->setShortcutContext(Qt::WidgetShortcut); list->addAction(deleteAction);
        connect(deleteButton,&QPushButton::clicked,this,[this]{ deletePreset(); });
        connect(deleteAction,&QAction::triggered,this,[this]{ deletePreset(); });
        auto *hint=new QLabel("New preset copies the selected template into this library or into a separate file.");
        hint->setWordWrap(true); hint->setObjectName("muted"); side->addWidget(hint); split->addWidget(sidebar);
        auto *right=new QWidget; auto *body=new QVBoxLayout(right); body->setContentsMargins(0,0,0,0);
        name=new QLineEdit; name->setPlaceholderText("Preset name"); body->addWidget(name);
        auto *advanced=new QWidget; auto *advancedLayout=new QVBoxLayout(advanced); advancedLayout->setContentsMargins(12,12,12,12);
        auto *searchRow=new QHBoxLayout;
        search=new QLineEdit; search->setPlaceholderText("Search settings: encoder, crop, language, bitrate…"); search->setClearButtonEnabled(true); searchRow->addWidget(search,1);
        auto *addSetting=new QPushButton("Add setting…"); searchRow->addWidget(addSetting); advancedLayout->addLayout(searchRow);
        auto *help=new QLabel("Double-click a value to edit. Right-click a list to add, copy, or remove entries. Suggested values come from your library.");
        help->setWordWrap(true); help->setObjectName("muted"); advancedLayout->addWidget(help);
        tabs=new QTabWidget;
        easy=new EasyView([this]{ return selected; },[this](const QString &key,const QJsonValue &value){
            if(loading || selectedIndex<0 || selected.value(key)==value) return;
            selected[key]=value; markChanged();
        },[this](const QString &key){ return suggestions.value(key); });
        tabs->addTab(easy,"Easy view");
        settings=new QTreeWidget; settings->setColumnCount(2); settings->setHeaderLabels({"Setting","Value"});
        settings->setAlternatingRowColors(true); settings->setUniformRowHeights(true);
        settings->setItemDelegate(new SettingDelegate(settings));
        settings->setEditTriggers(QAbstractItemView::DoubleClicked|QAbstractItemView::EditKeyPressed);
        settings->header()->setSectionResizeMode(0,QHeaderView::Interactive); settings->header()->setStretchLastSection(true); settings->setColumnWidth(0,340);
        settings->setContextMenuPolicy(Qt::CustomContextMenu); advancedLayout->addWidget(settings,1); tabs->addTab(advanced,"Advanced view");
        preview=new QPlainTextEdit; preview->setReadOnly(true); preview->setObjectName("preview"); tabs->addTab(preview,"Selected preset JSON");
        body->addWidget(tabs,1); split->addWidget(right); split->setSizes({260,900}); layout->addWidget(split,1); setCentralWidget(root);
        previewTimer.setSingleShot(true); previewTimer.setInterval(250);
        connect(&previewTimer,&QTimer::timeout,this,[this]{ updatePreview(); });
        connect(open,&QPushButton::clicked,this,[this]{ openFile(); });
        connect(addHere,&QAction::triggered,this,[this]{ newPreset(false); }); connect(newFile,&QAction::triggered,this,[this]{ newPreset(true); });
        connect(saveButton,&QPushButton::clicked,this,[this]{ save(); }); connect(saveAsButton,&QPushButton::clicked,this,[this]{ save(true); });
        connect(addSetting,&QPushButton::clicked,this,[this]{ addMissingSetting(); });
        connect(list,&QListWidget::currentRowChanged,this,[this](int row){ select(row); });
        connect(name,&QLineEdit::textEdited,this,[this](const QString &text){
            if(loading || selectedIndex<0) return; selected["PresetName"]=text; markChanged(); list->item(selectedIndex)->setText(text);
        });
        connect(settings,&QTreeWidget::itemChanged,this,[this](QTreeWidgetItem *item,int col){ editItem(item,col); });
        connect(settings,&QTreeWidget::customContextMenuRequested,this,[this](const QPoint &p){ arrayMenu(p); });
        connect(search,&QLineEdit::textChanged,this,[this]{ filter(); });
        connect(tabs,&QTabWidget::currentChanged,this,[this](int index){
            if(index==0) easy->refresh(); else if(index==1) rebuild(); updatePreview();
        });
        name->setEnabled(false); settings->setEnabled(false); search->setEnabled(false); validate(); updateTitle();
        statusBar()->showMessage("Ready · Changes stay in memory until Save");
    }
    bool loadFile(const QString &filePath) {
        QFile file(filePath); if(!file.open(QIODevice::ReadOnly)) { warn(file.errorString()); return false; }
        QString error; PresetDocument candidate; if(!candidate.load(file.readAll(),error)) { warn(error); return false; }
        document=candidate; path=filePath; dirty=false; selectedDirty=false; selectedIndex=-1;
        suggestions.clear(); examples.clear();
        QMap<QString,QMap<QString,QStringList>> perEncoder;
        for(const auto &p:document.presets()) {
            gather(p);
            for(const auto &key:{"VideoPreset","VideoTune","VideoProfile","VideoLevel"}) {
                const auto value=p.value(key); if(!value.isString()) continue;
                auto &list=perEncoder[p.value("VideoEncoder").toString()][key]; if(!list.contains(value.toString())) list.append(value.toString());
            }
            for(auto it=p.begin();it!=p.end();++it) if(!examples.contains(it.key())) examples[it.key()]=it.value();
        }
        examples["PresetDescription"]=QString();
        easy->setEncoderOptions(perEncoder);
        easy->setAudioTemplate(examples.value("AudioList").toArray().isEmpty()?QJsonObject():examples.value("AudioList").toArray().first().toObject());
        refresh(); return true;
    }
protected:
    void closeEvent(QCloseEvent *event) override { if(discard()) event->accept(); else event->ignore(); }
private:
    void warn(const QString &s) { QMessageBox::warning(this,"Preset Studio",s); }
    void commit() { if(selectedDirty && selectedIndex>=0) document.replace(selectedIndex,selected); selectedDirty=false; }
    void markChanged() { dirty=true; selectedDirty=true; previewStale=true; validate(); updateTitle(); if(tabs->currentIndex()==2) previewTimer.start(); }
    void validate() {
        const bool valid=!document.json.isNull() && (selectedIndex>=0?!selected["PresetName"].toString().trimmed().isEmpty():list->count()==0);
        saveButton->setEnabled(valid); saveAsButton->setEnabled(valid); deleteButton->setEnabled(selectedIndex>=0);
    }
    void updateTitle() { setWindowTitle(QString(dirty?"* ":"")+"Preset Studio — HandBrake Preset Creator"); }
    void updatePreview() {
        if(tabs->currentIndex()!=2 || !previewStale) return;
        preview->setPlainText(QString::fromUtf8(QJsonDocument(selected).toJson(QJsonDocument::Indented))); previewStale=false;
    }
    void refresh(int row=0) {
        loading=true; selectedIndex=-1; list->clear(); for(const auto &p:document.presets()) list->addItem(p["PresetName"].toString());
        loading=false; list->setCurrentRow(row); source->setText(path.isEmpty()?"New preset file · based on your template":QDir::toNativeSeparators(path)); updateTitle();
        if(list->count()==0) {
            selected={}; selectedIndex=-1; selectedDirty=false; name->clear(); name->setEnabled(false);
            name->setPlaceholderText("This library is empty · Open a template to create a preset");
            settings->clear(); settings->setEnabled(false); search->setEnabled(false); easy->setEnabled(false); easy->refresh();
            preview->clear(); previewStale=false; validate();
        }
    }
    void select(int row) {
        if(loading || row<0) return; commit(); selectedIndex=row; selected=document.presets().at(row);
        loading=true; name->setText(selected["PresetName"].toString()); name->setPlaceholderText("Preset name"); loading=false;
        name->setEnabled(true); settings->setEnabled(true); search->setEnabled(true); rebuild();
        easy->setEnabled(true); easy->resetTrack(); easy->refresh(); previewStale=true; updatePreview(); validate();
    }
    static QString category(const QString &key) {
        if(key.startsWith("Audio")) return "Audio"; if(key.startsWith("Subtitle")) return "Subtitles";
        if(key.startsWith("Video") || key.startsWith("x264")) return "Video";
        if(key.startsWith("Picture")) {
            for(const auto &word:{"Deblock","Deinterlace","CombDetect","Denoise","Sharpen","Detelecine","Colorspace","ChromaSmooth","Deband"})
                if(key.contains(QLatin1String(word))) return "Filters";
            return "Dimensions";
        }
        return "General";
    }
    void gather(const QJsonObject &object) {
        for(auto it=object.begin();it!=object.end();++it) {
            if(it.value().isString() && !it.value().toString().isEmpty() && it.value().toString().size()<100) {
                auto &values=suggestions[it.key()]; if(!values.contains(it.value().toString())) values.append(it.value().toString());
            }
            if(it.value().isArray()) for(const auto &v:it.value().toArray()) if(v.isObject()) gather(v.toObject());
        }
    }
    void addMissingSetting() {
        if(selectedIndex<0) return;
        QStringList missing;
        for(auto it=examples.begin();it!=examples.end();++it)
            if(!selected.contains(it.key()) && it.key()!="ChildrenArray" && it.key()!="Folder" && it.key()!="Type") missing.append(it.key());
        if(missing.isEmpty()) { statusBar()->showMessage("This preset already contains every setting found in your library."); return; }
        bool ok=false;
        const auto key=QInputDialog::getItem(this,"Add setting","Parameter from this library (initial value copied from a template)",missing,0,false,&ok);
        if(!ok) return; selected[key]=examples[key]; markChanged(); search->clear(); rebuild();
    }
    void rebuild() {
        loading=true; settings->setUpdatesEnabled(false); settings->clear(); QMap<QString,QTreeWidgetItem *> groups;
        for(const auto &title:{"General","Video","Dimensions","Filters","Audio","Subtitles"}) {
            auto *group=new QTreeWidgetItem(settings,{title}); group->setFlags(Qt::ItemIsEnabled); groups[title]=group;
            auto font=group->font(0); font.setBold(true); group->setFont(0,font); group->setExpanded(true);
        }
        for(auto it=selected.begin();it!=selected.end();++it) if(it.key()!="PresetName") addField(groups[category(it.key())],it.key(),it.value(),QVariantList{it.key()});
        settings->setUpdatesEnabled(true); loading=false; filter();
    }
    void addField(QTreeWidgetItem *parent,const QString &label,const QJsonValue &value,const QVariantList &parts) {
        auto *item=new QTreeWidgetItem(parent,{label}); item->setData(1,Qt::UserRole,int(value.type())); item->setData(1,PathRole,parts);
        item->setToolTip(0,parts.first().toString());
        if(value.isArray()) {
            const auto array=value.toArray(); item->setText(1,QString::number(array.size())+" entries");
            for(int i=0;i<array.size();++i) { auto next=parts; next.append(i); addField(item,QString("Entry %1").arg(i+1),array[i],next); }
            item->setExpanded(parts.first()=="AudioList");
        } else if(value.isObject()) {
            const auto object=value.toObject(); item->setText(1,"Object");
            for(auto it=object.begin();it!=object.end();++it) { auto next=parts; next.append(it.key()); addField(item,it.key(),it.value(),next); } item->setExpanded(true);
        } else {
            item->setText(1,value.isBool()?(value.toBool()?"true":"false"):value.isDouble()?QString::number(value.toDouble(),'g',17):value.isNull()?"null":value.toString());
            const bool structural=parts.size()==1 && (label=="Folder" || label=="FolderOpen" || label=="Type");
            if(!structural && !value.isNull()) item->setFlags(item->flags()|Qt::ItemIsEditable);
            else item->setToolTip(1,"Library structure metadata is retained automatically.");
            if(value.isString()) item->setData(1,Qt::UserRole+2,suggestions.value(label));
        }
    }
    static QJsonValue at(QJsonValue value,const QVariantList &parts,int depth=0) {
        if(depth==parts.size()) return value;
        return at(value.isArray()?value.toArray().at(parts[depth].toInt()):value.toObject().value(parts[depth].toString()),parts,depth+1);
    }
    static QJsonValue put(QJsonValue value,const QVariantList &parts,const QJsonValue &replacement,int depth=0) {
        if(depth==parts.size()) return replacement;
        if(value.isArray()) { auto array=value.toArray(); const int i=parts[depth].toInt(); array[i]=put(array[i],parts,replacement,depth+1); return array; }
        auto object=value.toObject(); const auto key=parts[depth].toString(); object[key]=put(object[key],parts,replacement,depth+1); return object;
    }
    void editItem(QTreeWidgetItem *item,int column) {
        if(loading || column!=1 || !(item->flags() & Qt::ItemIsEditable)) return;
        const auto parts=item->data(1,PathRole).toList(); if(parts.isEmpty()) return;
        const auto old=at(selected,parts); const auto text=item->text(1); QJsonValue value;
        if(old.isBool()) value=(text=="true");
        else if(old.isDouble()) { bool valid=false; const double n=QLocale::c().toDouble(text,&valid); if(!valid || !std::isfinite(n)) return; value=n; }
        else value=text;
        if(value==old) return; selected=put(selected,parts,value).toObject(); markChanged();
    }
    bool filterItem(QTreeWidgetItem *item,const QString &text) {
        bool match=item->text(0).contains(text,Qt::CaseInsensitive)||item->text(1).contains(text,Qt::CaseInsensitive);
        for(int i=0;i<item->childCount();++i) match=filterItem(item->child(i),text)||match;
        item->setHidden(!match); if(!text.isEmpty() && match) item->setExpanded(true); return match;
    }
    void filter() { for(int i=0;i<settings->topLevelItemCount();++i) filterItem(settings->topLevelItem(i),search->text()); }
    void arrayMenu(const QPoint &point) {
        auto *item=settings->itemAt(point); if(!item) return;
        auto parts=item->data(1,PathRole).toList(); if(parts.isEmpty() || parts.first()=="ChildrenArray") return;
        const auto value=at(selected,parts); const bool entry=!value.isArray();
        if(entry) { auto pp=parts; pp.removeLast(); if(pp.isEmpty() || !at(selected,pp).isArray()) return; }
        QMenu menu; QAction *append=nullptr,*duplicate=nullptr,*remove=nullptr;
        if(!entry) append=menu.addAction("Add entry…"); else { duplicate=menu.addAction("Duplicate entry"); remove=menu.addAction("Remove entry"); }
        auto *action=menu.exec(settings->viewport()->mapToGlobal(point)); if(!action) return;
        if(entry) {
            const int index=parts.takeLast().toInt(); auto array=at(selected,parts).toArray();
            if(action==remove) array.removeAt(index); else if(action==duplicate) array.insert(index+1,array[index]); selected=put(selected,parts,array).toObject();
        } else if(action==append) {
            auto array=value.toArray(); QJsonValue added;
            if(!array.isEmpty() && array.first().isObject()) added=array.first();
            else if(parts.last()=="AudioList") {
                bool ok=false; const auto text=QInputDialog::getMultiLineText(this,"Add audio track","Audio track JSON object:","{}",&ok); if(!ok) return;
                QJsonParseError error; const auto parsed=QJsonDocument::fromJson(text.toUtf8(),&error);
                if(error.error!=QJsonParseError::NoError || !parsed.isObject()) { warn("Expected a JSON object."); return; } added=parsed.object();
            } else {
                bool ok=false; const auto text=QInputDialog::getText(this,"Add entry","Value",QLineEdit::Normal,{},&ok); if(!ok) return;
                if(!array.isEmpty() && array.first().isDouble()) { bool valid=false; const double n=text.toDouble(&valid); if(!valid) { warn("Expected a number."); return; } added=n; }
                else if(!array.isEmpty() && array.first().isBool()) { if(text!="true" && text!="false") { warn("Enter true or false."); return; } added=(text=="true"); }
                else added=text;
            }
            array.append(added); selected=put(selected,parts,array).toObject();
        }
        markChanged(); rebuild();
    }
    bool discard() {
        settings->setFocus(); if(!dirty) return true;
        auto result=QMessageBox::question(this,"Unsaved changes","Save your changes before continuing?",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Save);
        if(result==QMessageBox::Save) return save(); return result==QMessageBox::Discard;
    }
    void openFile() {
        if(!discard()) return; const auto file=QFileDialog::getOpenFileName(this,"Open HandBrake presets",{},"HandBrake presets (*.json)"); if(!file.isEmpty()) loadFile(file);
    }
    void newPreset(bool separate) {
        if(separate && !discard()) return;
        if(selectedIndex<0) {
            const auto file=QFileDialog::getOpenFileName(this,"Choose a HandBrake template",{},"HandBrake presets (*.json)");
            if(file.isEmpty()) return;
            const auto previous=document; const auto previousPath=path; const bool previousDirty=dirty;
            if(!loadFile(file)) return;
            if(selectedIndex<0) { warn("Choose a template file containing at least one preset."); document=previous; path=previousPath; dirty=previousDirty; refresh(-1); return; }
            if(!separate && !previous.json.isNull()) {
                document.createFrom(0); const auto p=document.presets().first();
                document=previous; path=previousPath; document.append(p); selectedIndex=-1; selectedDirty=false; dirty=true; refresh();
                name->setFocus(); name->selectAll(); statusBar()->showMessage("Preset added to this library from the chosen template · Save to update the JSON file"); return;
            }
        }
        commit(); const int index=selectedIndex;
        if(separate) { document.createFrom(index); path.clear(); dirty=true; refresh(); }
        else { const int added=document.addFrom(index); dirty=true; refresh(added); }
        name->setFocus(); name->selectAll(); statusBar()->showMessage(separate?"New preset file · Save to choose its location":"Preset added to library · Save to update the JSON file");
    }
    void removeSelected() {
        if(selectedIndex<0) return;
        const int index=selectedIndex;
        if(!document.remove(index)) return;
        selectedDirty=false; selectedIndex=-1; selected={}; dirty=true;
        const int count=document.presets().size(); refresh(count==0?-1:std::min(index,count-1));
        statusBar()->showMessage(count==0?"Last preset deleted · Save to write the empty library":"Preset deleted · Save to update the JSON file");
    }
    void deletePreset() {
        if(selectedIndex<0) return;
        settings->setFocus();
        const auto answer=QMessageBox::question(this,"Delete preset?",
            QString("Delete “%1” from this library?\n\nThe JSON file changes only when you save.").arg(selected.value("PresetName").toString()),
            QMessageBox::Yes|QMessageBox::No,QMessageBox::No);
        if(answer==QMessageBox::Yes) removeSelected();
    }
    bool save(bool saveAs=false) {
        settings->setFocus();
        if(selectedIndex>=0 && !easy->validationError().isEmpty()) { warn(easy->validationError()); return false; }
        commit(); if(document.json.isNull()) return false;
        for(const auto &p:document.presets()) if(p["PresetName"].toString().trimmed().isEmpty()) { warn("Every preset needs a name."); return false; }
        auto target=path;
        if(saveAs || target.isEmpty()) target=QFileDialog::getSaveFileName(this,"Save HandBrake presets",path.isEmpty()?"custom-preset.json":path,"HandBrake presets (*.json)");
        if(target.isEmpty()) return false;
        if(!target.endsWith(".json",Qt::CaseInsensitive)) { target+=".json"; if(QFileInfo::exists(target) && QMessageBox::question(this,"Replace file?",target+" already exists. Replace it?")!=QMessageBox::Yes) return false; }
        QSaveFile file(target); const auto bytes=document.json.toJson(QJsonDocument::Indented);
        if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit()) { warn(file.errorString()); return false; }
        path=target; dirty=false; source->setText(QDir::toNativeSeparators(path)); updateTitle(); statusBar()->showMessage("Saved · HandBrake preset library updated"); return true;
    }
};
