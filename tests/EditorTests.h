#pragma once
#include "../editor.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QTemporaryDir>

class EditorTests : public Editor {
public:
    int testEditing() {
        if(selectedIndex<0) return 20;
        const auto original=document.json; const auto originalPreset=selected; int count=0;
        QElapsedTimer timer; timer.start();
        for(int i=0;i<1000;++i) name->textEdited(QString("Typing test %1").arg(i));
        qInfo() << "1000 name edits (ms):" << timer.elapsed();
        if(document.json!=original || !preview->toPlainText().isEmpty()) return 24;
        QTreeWidgetItemIterator it(settings);
        while(*it) {
            auto *item=*it; ++it; if(!(item->flags() & Qt::ItemIsEditable)) continue;
            const auto parts=item->data(1,PathRole).toList(); if(parts.isEmpty()) continue;
            const int type=item->data(1,Qt::UserRole).toInt();
            if(type==QJsonValue::Bool) { item->setText(1,item->text(1)=="true"?"false":"true"); ++count; }
            else if(type==QJsonValue::Double) { item->setText(1,"23"); ++count; }
            else if(type==QJsonValue::String && parts[0].toString()!="VideoEncoder") { item->setText(1,"test value"); ++count; }
        }
        if(count<20 || document.json!=original || !selectedDirty || selected==originalPreset) return 21;
        qInfo() << "Typed setting edits:" << count << "presets in library:" << document.presets().size();
        const auto edited=selected; commit(); if(document.presets()[selectedIndex]!=edited) return 22;
        document.replace(selectedIndex,originalPreset); if(document.json!=original) return 23;
        selected=originalPreset; dirty=false; selectedDirty=false; return 0;
    }
    int testEasyView() {
        if(selectedIndex<0) return 30;
        const auto original=document.json;
        // Every preset must survive loading and switching between views unchanged.
        for(int i=0;i<list->count();++i) {
            if(i%10==0) qInfo()<<"Checking preset"<<i;
            list->setCurrentRow(i); tabs->setCurrentIndex(0); easy->refresh(); tabs->setCurrentIndex(1);
        }
        if(document.json!=original || dirty) return 31;
        list->setCurrentRow(0); tabs->setCurrentIndex(0); const auto originalPreset=selected;
        auto *quality=easy->findChild<QDoubleSpinBox *>("easy.VideoQualitySlider");
        auto *rate=easy->findChild<QComboBox *>("easy.VideoQualityType");
        auto *bitrate=easy->findChild<QDoubleSpinBox *>("easy.VideoAvgBitrate");
        auto *format=easy->findChild<QComboBox *>("easy.FileFormat");
        if(!quality || !rate || !bitrate || !format) return 32;
        quality->setValue(21);
        if(selected["VideoQualitySlider"].toDouble()!=21) return 33;
        tabs->setCurrentIndex(1);
        QTreeWidgetItemIterator it(settings); QTreeWidgetItem *advancedQuality=nullptr;
        while(*it) { if((*it)->text(0)=="VideoQualitySlider") advancedQuality=*it; ++it; }
        if(!advancedQuality || advancedQuality->text(1).toDouble()!=21) return 34;
        advancedQuality->setText(1,"22"); tabs->setCurrentIndex(0);
        if(quality->value()!=22) return 35;
        const int rateIndex=rate->findData(1); rate->setCurrentIndex(rateIndex); rate->activated(rateIndex);
        if(selected["VideoQualityType"].toInt()!=1 || quality->isEnabled() || !bitrate->isEnabled()) return 36;
        bitrate->setValue(3500); if(selected["VideoAvgBitrate"].toInt()!=3500) return 37;
        const int mp4=format->findData("av_mp4"); format->setCurrentIndex(mp4); format->activated(mp4);
        if(selected["FileFormat"]!="av_mp4" || !easy->findChild<QCheckBox *>("easy.Optimize")->isEnabled()) return 38;
        auto *crop=easy->findChild<QComboBox *>("easy.PictureCropMode");
        crop->setCurrentIndex(crop->findData(3)); crop->activated(crop->currentIndex());
        auto *top=easy->findChild<QDoubleSpinBox *>("easy.PictureTopCrop"); top->setValue(20);
        if(selected["PictureCropMode"].toInt()!=3 || selected["PictureTopCrop"].toInt()!=20) return 39;
        auto *languages=easy->findChild<QLineEdit *>("easy.AudioLanguageList");
        languages->setText("deu, eng, jpn"); languages->setModified(true); languages->editingFinished();
        if(selected["AudioLanguageList"].toArray()!=QJsonArray{"deu","eng","jpn"}) return 40;
        auto *audio=easy->findChild<QDoubleSpinBox *>("track.AudioBitrate"); audio->setValue(256);
        if(selected["AudioList"].toArray()[0].toObject()["AudioBitrate"].toInt()!=256) return 41;
        const auto audioBefore=selected["AudioList"].toArray();
        for(auto *button:easy->findChildren<QPushButton *>()) if(button->text()=="Add / duplicate") button->click();
        if(selected["AudioList"].toArray().size()!=audioBefore.size()+1 || selected["AudioList"].toArray().last()!=audioBefore.first()) return 42;
        for(auto *button:easy->findChildren<QPushButton *>()) if(button->text()=="Remove") button->click();
        if(selected["AudioList"].toArray()!=audioBefore) return 43;
        const auto edited=selected; commit();
        if(document.presets()[0]!=edited) return 44;
        document.replace(0,originalPreset); if(document.json!=original) return 45;
        selected=originalPreset; dirty=false; selectedDirty=false;
        qInfo()<<"Easy/Advanced synchronization, all"<<list->count()<<"preset no-op round trips, numeric enum types, dependencies, nested audio and language edits passed";
        return 0;
    }
    int testRequestedFeatures() {
        if(selectedIndex<0) return 50;
        const auto original=document.json; const auto originalPreset=selected;
        auto choose=[this](const QString &name,const QVariant &data) {
            auto *combo=easy->findChild<QComboBox *>(name); if(!combo) return false;
            const int index=combo->findData(data); if(index<0) return false;
            combo->setCurrentIndex(index); combo->activated(index); return true;
        };
        if(!choose("resolutionLimit",QSize(3840,2160)) || selected["PictureWidth"].toInt()!=3840 || selected["PictureHeight"].toInt()!=2160) return 51;
        if(easy->findChild<QDoubleSpinBox *>("easy.PictureWidth")->isEnabled()) return 52;
        if(!choose("resolutionLimit",QSize(-1,-1)) || !easy->findChild<QDoubleSpinBox *>("easy.PictureWidth")->isEnabled()) return 53;
        if(!choose("easy.PicturePAR","off") || easy->findChild<QDoubleSpinBox *>("easy.PicturePARWidth")->isEnabled()) return 54;
        if(!choose("easy.PicturePAR","custom") || !easy->findChild<QDoubleSpinBox *>("easy.PicturePARHeight")->isEnabled()) return 55;
        if(!choose("easy.PicturePadMode","none") || easy->findChild<QDoubleSpinBox *>("easy.PicturePadLeft")->isEnabled()) return 56;
        if(!choose("easy.PicturePadMode","letterbox") || easy->findChild<QDoubleSpinBox *>("easy.PicturePadTop")->isEnabled()) return 57;
        if(!choose("easy.PicturePadMode","custom") || !easy->findChild<QDoubleSpinBox *>("easy.PicturePadLeft")->isEnabled()) return 58;
        if(!choose("fillColor","custom")) return 59;
        auto *rgb=easy->findChild<QLineEdit *>("customFillColor"); if(rgb->isHidden()) return 60;
        rgb->setText("17, 34, 51"); rgb->setModified(true); rgb->editingFinished();
        if(selected["PicturePadColor"]!="0x112233") return 61;
        rgb->setText("999, -1, 51"); rgb->setModified(true); rgb->editingFinished();
        if(selected["PicturePadColor"]!="0x112233") return 62;
        if(!choose("fillColor","white") || selected["PicturePadColor"]!="white" || !rgb->isHidden()) return 63;
        const QList<QPair<QString,QString>> customFilters{{"PictureDetelecine","PictureDetelecineCustom"},{"PictureCombDetectPreset","PictureCombDetectCustom"},
            {"PictureChromaSmoothPreset","PictureChromaSmoothCustom"},{"PictureDeblockPreset","PictureDeblockCustom"},{"PictureColorspacePreset","PictureColorspaceCustom"}};
        for(const auto &filter:customFilters) {
            if(!choose("easy."+filter.first,"custom")) return 64;
            auto *line=easy->findChild<QLineEdit *>("easy."+filter.second); if(!line || line->isHidden()) return 65;
            line->setText("strength=1"); line->setModified(true); line->editingFinished();
            if(selected[filter.second]!="strength=1") return 66;
            if(!choose("easy."+filter.first,"off") || !line->isHidden()) return 67;
        }
        if(!choose("easy.PictureDenoiseFilter","nlmeans") || !choose("easy.PictureDenoisePreset","custom") || easy->findChild<QLineEdit *>("easy.PictureDenoiseCustom")->isHidden()) return 68;
        if(!choose("easy.PictureDeinterlaceFilter","decomb") || !choose("easy.PictureDeinterlacePreset","eedi2bob")) return 69;
        if(!choose("easy.PictureDeinterlaceFilter","bwdif") || easy->findChild<QComboBox *>("easy.PictureDeinterlacePreset")->findData("eedi2bob")>=0) return 70;
        if(!choose("easy.PictureSharpenFilter","unsharp") || easy->findChild<QComboBox *>("easy.PictureSharpenTune")->findData("ultrafine")<0) return 71;
        auto *encoders=easy->findChild<QComboBox *>("easy.VideoEncoder"); QString nvenc;
        for(int i=0;i<encoders->count();++i) if(encoders->itemData(i).toString().startsWith("nvenc")) { nvenc=encoders->itemData(i).toString(); break; }
        if(nvenc.isEmpty() || !choose("easy.VideoEncoder",nvenc)) return 72;
        if(easy->findChild<QComboBox *>("easy.VideoPreset")->findData("fastest")<0 || easy->findChild<QComboBox *>("easy.VideoPreset")->findData("placebo")>=0) return 73;
        if(easy->findChild<QCheckBox *>("easy.VideoMultiPass")->isEnabled() || easy->findChild<QComboBox *>("easy.VideoTune")->isEnabled()) return 74;
        if(!choose("easy.VideoEncoder","x264") || !choose("easy.VideoQualityType",1) || !easy->findChild<QCheckBox *>("easy.VideoMultiPass")->isEnabled()) return 75;
        auto *frame=easy->findChild<QComboBox *>("easy.VideoFramerateMode"); if(frame->count()!=2 || frame->findData("pfr")>=0) return 76;
        if(easy->findChild<QDoubleSpinBox *>("easy.VideoQualitySlider")->decimals()!=0) return 77;
        const auto edited=selected; commit(); if(document.presets()[selectedIndex]!=edited) return 78;
        document.replace(selectedIndex,originalPreset); if(document.json!=original) return 79;
        Theme::apply(true); if(!Theme::dark()) return 80; Theme::apply(false); if(Theme::dark()) return 81;
        if(document.json!=original) return 82;
        selected=originalPreset; dirty=false; selectedDirty=false;
        qInfo()<<"Themes, resolution presets, aspect/padding dependencies, RGB validation, filter custom fields, scoped options, CPU/NVENC controls and integer quality passed";
        return 0;
    }
    int testDeletion() {
        if(selectedIndex<0) return 90;
        const auto original=document.json; const auto originalPath=path; const int originalCount=list->count();
        auto answer=[](QMessageBox::StandardButton button) {
            QTimer::singleShot(0,[button]{
                for(auto *widget:QApplication::topLevelWidgets())
                    if(auto *dialog=qobject_cast<QMessageBox *>(widget)) if(dialog->isVisible()) dialog->button(button)->click();
            });
        };
        answer(QMessageBox::No); deletePreset();
        if(document.json!=original || dirty || list->count()!=originalCount) return 91;
        PresetDocument expected=document; expected.remove(selectedIndex);
        name->textEdited("Unsaved edit on deleted preset");
        answer(QMessageBox::Yes); deletePreset();
        if(document.json!=expected.json || !dirty || list->count()!=originalCount-1 || selectedDirty) return 92;
        commit(); if(document.json!=expected.json) return 93;
        QString error;
        if(!document.load(R"({"PresetList":[{"PresetName":"Last","VideoEncoder":"x264"}],"VersionMajor":60})",error)) return 94;
        selectedIndex=-1; selectedDirty=false; dirty=false; refresh();
        answer(QMessageBox::Yes); deletePreset();
        if(list->count()!=0 || selectedIndex!=-1 || deleteButton->isEnabled() || !saveButton->isEnabled() || easy->isEnabled()) return 95;
        QTemporaryDir temporary; path=temporary.filePath("empty-library.json");
        if(!save()) return 96;
        QFile file(path); if(!file.open(QIODevice::ReadOnly)) return 97;
        PresetDocument reopened;
        if(!reopened.load(file.readAll(),error) || !reopened.presets().isEmpty() || reopened.json.object()["VersionMajor"].toInt()!=60) return 98;
        document.json=original; path=originalPath; selectedDirty=false; dirty=false; selectedIndex=-1; refresh();
        qInfo()<<"Deletion confirmation/cancel, unsaved-cache handling, neighboring selection, last-preset state and empty-library save/reopen passed";
        return 0;
    }
};
