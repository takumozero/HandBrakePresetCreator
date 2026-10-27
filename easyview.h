#pragma once
#include "handbrakeoptions.h"
#include <QCheckBox>
#include <QComboBox>
#include <QColor>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QSignalBlocker>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTabWidget>
#include <QVBoxLayout>
#include <functional>
#include <algorithm>
#include <cmath>

// Original Qt implementation inspired by HandBrake's Windows settings layout.
// Both views read/write the Editor's selected preset; loading never changes JSON.
class EasyView : public QTabWidget {
    using Choice=QPair<QString,QJsonValue>;
    std::function<QJsonObject()> read;
    std::function<void(const QString &,const QJsonValue &)> write;
    std::function<QStringList(const QString &)> options;
    QList<std::function<void()>> bindings;
    QList<std::function<void()>> choiceBindings;
    QMap<QString,QMap<QString,QStringList>> encoderOptions;
    QComboBox *resolution,*fillColor;
    QLineEdit *customColor;
    QFormLayout *paddingForm;
    QString lastCustomColor="000000";
    bool customResolution=false;
    QComboBox *audioTrack;
    QWidget *audioFields;
    QJsonObject audioTemplate;
    QMap<QString,QWidget *> controls;
    bool syncing=false;
    int trackIndex=0;
    QVBoxLayout *page(const QString &title) {
        auto *scroll=new QScrollArea; scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
        auto *content=new QWidget; auto *layout=new QVBoxLayout(content);
        layout->setContentsMargins(20,18,20,20); layout->setSpacing(18); layout->setAlignment(Qt::AlignTop);
        scroll->setWidget(content); addTab(scroll,title); return layout;
    }
    QFormLayout *group(QVBoxLayout *parent,const QString &title) {
        auto *box=new QGroupBox(title); auto *form=new QFormLayout(box); form->setSpacing(12);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow); parent->addWidget(box); return form;
    }
    static void note(QVBoxLayout *layout,const QString &text) {
        auto *label=new QLabel(text); label->setWordWrap(true); label->setObjectName("muted"); layout->addWidget(label);
    }
    QJsonValue value(const QString &key,bool track=false) const {
        if(!track) return read().value(key);
        const auto array=read().value("AudioList").toArray();
        return trackIndex>=0 && trackIndex<array.size()?array[trackIndex].toObject().value(key):QJsonValue();
    }
    void set(const QString &key,const QJsonValue &v,bool track=false) {
        if(syncing || !isEnabled() || value(key,track)==v) return;
        if(track) {
            auto array=read().value("AudioList").toArray(); if(trackIndex<0 || trackIndex>=array.size()) return;
            auto object=array[trackIndex].toObject(); object[key]=v; array[trackIndex]=object; write("AudioList",array);
        } else write(key,v);
        if(key=="VideoEncoder") {
            const auto enc=v.toString();
            for(const auto &option:{"VideoPreset","VideoTune","VideoProfile","VideoLevel"}) {
                const auto allowed=available(option);
                if(!allowed.isEmpty() && !allowed.contains(read().value(option).toString())) {
                    const auto fallback=QString(option)=="VideoPreset" && allowed.contains("medium")?QString("medium"):allowed.first();
                    write(option,fallback);
                }
            }
            if(!HandBrakeOptions::multipass(enc,read().value("VideoQualityType").toInt()==2)) {
                if(read().contains("VideoMultiPass")) write("VideoMultiPass",false);
                if(read().contains("VideoTurboMultiPass")) write("VideoTurboMultiPass",false);
            }
            if(read().contains("VideoOptionExtra")) write("VideoOptionExtra",QString());
        }
        if(key=="VideoEncoder" || key=="PictureDeinterlaceFilter" || key=="PictureDenoiseFilter" || key=="PictureSharpenFilter") {
            if(key!="VideoEncoder") {
                const auto prefix=key.left(key.size()-6);
                for(const auto &suffix:{"Preset","Tune"}) {
                    const auto parameter=prefix+suffix; const auto allowed=available(parameter);
                    if(!allowed.isEmpty() && !allowed.contains(read().value(parameter).toString()))
                        write(parameter,allowed.contains("default")?QString("default"):allowed.contains("medium")?QString("medium"):allowed.first());
                }
            }
            for(const auto &binding:choiceBindings) binding();
        }
        dependencies();
    }
    QStringList available(const QString &key) const {
        const auto filter=HandBrakeOptions::filter(key,read()); if(!filter.isEmpty()) return filter;
        if(key=="VideoPreset" || key=="VideoTune" || key=="VideoProfile" || key=="VideoLevel") {
            const auto enc=read().value("VideoEncoder").toString();
            const auto builtIn=HandBrakeOptions::encoder(key,enc);
            return builtIn.isEmpty()?encoderOptions.value(enc).value(key):builtIn;
        }
        return options(key);
    }
    void tag(QWidget *widget,const QString &key,bool track=false) {
        widget->setObjectName((track?"track.":"easy.")+key);
        widget->setToolTip(key+" · Fields absent from this template are added only when edited.");
        controls[(track?"track.":"")+key]=widget;
    }
    void check(QFormLayout *form,const QString &label,const QString &key,bool track=false) {
        auto *widget=new QCheckBox(label); tag(widget,key,track); form->addRow("",widget);
        bindings.append([=,this]{ widget->setChecked(value(key,track).toBool()); });
        connect(widget,&QCheckBox::toggled,this,[=,this](bool v){ set(key,v,track); });
    }
    void number(QFormLayout *form,const QString &label,const QString &key,int decimals=0,bool track=false) {
        auto *widget=new QDoubleSpinBox; widget->setRange(key.startsWith("Picture") || key.endsWith("Bitrate")?0:-1000000,1000000); widget->setDecimals(decimals);
        widget->setSingleStep(decimals?0.5:1); widget->setKeyboardTracking(false); tag(widget,key,track); form->addRow(label,widget);
        bindings.append([=,this]{ widget->setValue(value(key,track).toDouble()); });
        connect(widget,&QDoubleSpinBox::valueChanged,this,[=,this](double v){ set(key,v,track); });
    }
    void line(QFormLayout *form,const QString &label,const QString &key,bool list=false,bool track=false) {
        auto *widget=new QLineEdit; tag(widget,key,track); form->addRow(label,widget);
        if(list) widget->setPlaceholderText("Comma-separated codes, e.g. deu, eng, jpn");
        else if(key.endsWith("Custom")) widget->setPlaceholderText("key=value:key=value · HandBrake filter expression");
        bindings.append([=,this]{
            if(list) { QStringList values; for(const auto &v:value(key,track).toArray()) values.append(v.toString()); widget->setText(values.join(", ")); }
            else widget->setText(value(key,track).toString());
        });
        connect(widget,&QLineEdit::editingFinished,this,[=,this]{
            if(!widget->isModified()) return;
            widget->setModified(false);
            if(list) { QJsonArray array; for(const auto &v:widget->text().split(',',Qt::SkipEmptyParts)) if(!v.trimmed().isEmpty()) array.append(v.trimmed()); set(key,array,track); }
            else set(key,widget->text(),track);
        });
    }
    void choice(QFormLayout *form,const QString &label,const QString &key,const QList<Choice> &choices={},bool track=false) {
        auto *widget=new QComboBox; tag(widget,key,track); form->addRow(label,widget);
        widget->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon); widget->setMinimumContentsLength(16);
        const auto binding=[=,this]{
            const auto values=available(key);
            const bool controlled=!HandBrakeOptions::filter(key,read()).isEmpty()
                || key=="VideoPreset" || key=="VideoTune" || key=="VideoProfile" || key=="VideoLevel";
            const bool frameMode=key=="VideoFramerateMode";
            if(!widget->property("optionsReady").toBool() || widget->property("libraryOptions").toStringList()!=values) {
                widget->clear();
                if(!controlled) for(const auto &entry:choices) widget->addItem(entry.first,entry.second.toVariant());
                if(!frameMode) for(const auto &raw:values) if(widget->findData(raw)<0) widget->addItem(friendly(key,raw),raw);
                widget->setProperty("optionsReady",true); widget->setProperty("libraryOptions",values);
            }
            const auto current=value(key,track);
            if(frameMode && current.toString()!="cfr" && current.toString()!="vfr") {
                widget->setPlaceholderText(current.isUndefined()?"Choose CFR or VFR":"Existing "+current.toString().toUpper()+" · choose CFR or VFR");
                widget->setCurrentIndex(-1); return;
            }
            int index=widget->findData(current.toVariant());
            if(index<0) { widget->addItem(current.isUndefined()?"Not set in template":current.isDouble()?QString::number(current.toDouble()):friendly(key,current.toString()),current.toVariant()); index=widget->count()-1; }
            widget->setCurrentIndex(index);
        };
        bindings.append(binding); choiceBindings.append(binding);
        connect(widget,&QComboBox::activated,this,[=,this](int index){ if(widget->itemData(index).isValid()) set(key,QJsonValue::fromVariant(widget->itemData(index)),track); });
    }
    static QString friendly(const QString &key,const QString &raw) {
        if(key=="VideoFramerate" && raw=="auto") return "Same as source";
        if(key=="VideoFramerateMode" && raw=="pfr") return "Peak framerate (existing preset)";
        if(raw.isEmpty()) return "None / default";
        if(raw=="auto") return "Automatic";
        if(raw=="off") return "Off";
        if(key=="FileFormat") {
            if(raw=="mp4" || raw=="av_mp4") return "MP4";
            if(raw=="mkv" || raw=="av_mkv") return "Matroska (MKV)";
            if(raw=="av_mov") return "QuickTime (MOV)";
        }
        if(key=="VideoEncoder") {
            const QMap<QString,QString> names{{"x264","H.264 (x264)"},{"x264_10bit","H.264 10-bit (x264)"},{"x265","H.265 (x265)"},
                {"x265_10bit","H.265 10-bit (x265)"},{"nvenc_h264","H.264 (NVIDIA NVENC)"},{"nvenc_h265","H.265 (NVIDIA NVENC)"},
                {"nvenc_h265_10bit","H.265 10-bit (NVIDIA NVENC)"},{"svt_av1","AV1 (SVT-AV1)"},{"svt_av1_10bit","AV1 10-bit (SVT-AV1)"}};
            return names.value(raw,raw);
        }
        if(key=="AudioEncoder" || key=="AudioEncoderFallback") {
            if(raw=="av_aac") return "AAC (FFmpeg)";
            if(raw=="copy") return "Auto passthrough";
            if(raw.startsWith("copy:")) return raw.mid(5).toUpper()+" passthrough";
        }
        const QMap<QString,QString> names{{"ultralight","Ultralight"},{"light","Light"},{"medium","Medium"},{"strong","Strong"},{"stronger","Stronger"},{"verystrong","Very strong"},
            {"verywide","Very wide"},{"verycoarse","Very coarse"},{"eedi2","EEDI2"},
            {"custom","Custom"},{"default","Default"},{"permissive","Less sensitive"},{"highmotion","High motion"},{"skip-spatial","Skip spatial check"},
            {"eedi2bob","EEDI2 Bob"},{"bt2020","BT.2020"},{"bt709","BT.709"},{"bt601-6-525","BT.601 SMPTE-C"},{"bt601-6-625","BT.601 EBU"},
            {"hqdn3d","HQDN3D"},{"nlmeans","NLMeans"},{"bm3d","BM3D"},{"yadif","Yadif"},{"bwdif","Bwdif"},{"decomb","Decomb"},{"lapsharp","LapSharp"},{"unsharp","Unsharp"}};
        if(names.contains(raw)) return names[raw];
        return raw.isEmpty()?raw:raw.left(1).toUpper()+raw.mid(1);
    }
    void dependencies() {
        auto enable=[this](const QString &key,bool enabled){ if(controls.contains(key)) controls[key]->setEnabled(enabled); };
        const int quality=read().value("VideoQualityType").toInt(2);
        enable("VideoQualitySlider",quality==2); enable("VideoAvgBitrate",quality==1);
        const auto enc=read().value("VideoEncoder").toString();
        if(auto *factor=qobject_cast<QDoubleSpinBox *>(controls.value("VideoQualitySlider"))) {
            int minimum=-1000000,maximum=1000000;
            if(enc.startsWith("x264") || enc.startsWith("x265")) {
                minimum=enc.contains("16bit")?-48:enc.contains("12bit")?-24:enc.contains("10bit")?-12:0; maximum=51;
            } else if(enc.startsWith("nvenc")) { minimum=1; maximum=enc.contains("av1")?63:51; }
            else if(enc.startsWith("svt_av1")) { minimum=0; maximum=70; }
            QSignalBlocker blocker(factor);
            const double existing=read().value("VideoQualitySlider").toDouble();
            factor->setRange(std::min(double(minimum),std::floor(existing)),std::max(double(maximum),std::ceil(existing)));
            factor->setValue(existing);
            factor->setToolTip(QString("%1 quality scale: %2 to %3. Existing imported values are preserved; Easy view edits whole numbers.").arg(enc).arg(minimum).arg(maximum));
            if(auto *form=qobject_cast<QFormLayout *>(factor->parentWidget()->layout()))
                if(auto *label=qobject_cast<QLabel *>(form->labelForField(factor))) label->setText(enc.startsWith("nvenc")?"Constant quality (CQ)":"Constant quality (RF)");
        }
        enable("VideoMultiPass",HandBrakeOptions::multipass(enc,quality==2));
        enable("VideoTurboMultiPass",quality==1 && (enc.startsWith("x264") || enc.startsWith("x265")) && read().value("VideoMultiPass").toBool());
        for(const auto &key:{"VideoPreset","VideoTune","VideoProfile","VideoLevel"}) {
            const auto list=available(key); enable(key,!list.isEmpty() && !(QString(key)=="VideoTune" && enc.startsWith("nvenc")));
        }
        if(enc.startsWith("nvenc") && enc.contains("av1")) { enable("VideoProfile",false); enable("VideoLevel",false); }
        const auto format=read().value("FileFormat").toString();
        const bool mp4=format=="av_mp4" || format=="mp4";
        enable("Optimize",mp4); enable("AlignAVStart",mp4); enable("Mp4iPodCompatible",mp4);
        const bool customCrop=read().value("PictureCropMode").toInt()==3;
        for(const auto &edge:{"Top","Bottom","Left","Right"}) enable(QString("Picture")+edge+"Crop",customCrop);
        if(audioFields) audioFields->setEnabled(!read().value("AudioList").toArray().isEmpty());
        const bool customPAR=read().value("PicturePAR")=="custom";
        enable("PicturePARWidth",customPAR); enable("PicturePARHeight",customPAR);
        if(resolution) { const bool custom=resolution->currentData().toSize()==QSize(-1,-1); enable("PictureWidth",custom); enable("PictureHeight",custom); }
        const auto padding=read().value("PicturePadMode").toString();
        for(const auto &edge:{"Top","Bottom","Left","Right"}) enable(QString("PicturePad")+edge,padding=="custom");
        if(fillColor) fillColor->setEnabled(padding!="none" && !padding.isEmpty());
        if(customColor && fillColor) paddingForm->setRowVisible(customColor,padding!="none" && !padding.isEmpty() && fillColor->currentData()=="custom");
        auto show=[this](const QString &key,bool visible) {
            auto *widget=controls.value(key); if(!widget) return;
            if(auto *form=qobject_cast<QFormLayout *>(widget->parentWidget()->layout())) form->setRowVisible(widget,visible);
        };
        show("PictureDetelecineCustom",read().value("PictureDetelecine")=="custom");
        show("PictureCombDetectCustom",read().value("PictureCombDetectPreset")=="custom");
        const auto deint=read().value("PictureDeinterlaceFilter").toString();
        show("PictureDeinterlacePreset",!deint.isEmpty() && deint!="off"); show("PictureDeinterlaceCustom",deint!="off" && !deint.isEmpty() && read().value("PictureDeinterlacePreset")=="custom");
        const auto denoise=read().value("PictureDenoiseFilter").toString();
        show("PictureDenoisePreset",!denoise.isEmpty() && denoise!="off"); show("PictureDenoiseTune",denoise=="nlmeans" && read().value("PictureDenoisePreset")!="custom");
        show("PictureDenoiseCustom",denoise!="off" && !denoise.isEmpty() && read().value("PictureDenoisePreset")=="custom");
        const auto sharpen=read().value("PictureSharpenFilter").toString();
        show("PictureSharpenPreset",!sharpen.isEmpty() && sharpen!="off"); show("PictureSharpenTune",!sharpen.isEmpty() && sharpen!="off" && read().value("PictureSharpenPreset")!="custom");
        show("PictureSharpenCustom",!sharpen.isEmpty() && sharpen!="off" && read().value("PictureSharpenPreset")=="custom");
        for(const auto &base:{"PictureChromaSmooth","PictureDeblock","PictureColorspace"}) {
            const QString prefix=base; const auto preset=read().value(prefix+"Preset").toString();
            show(prefix+"Tune",!preset.isEmpty() && preset!="off" && preset!="custom"); show(prefix+"Custom",preset=="custom");
        }
    }
public:
    EasyView(std::function<QJsonObject()> getter,std::function<void(const QString &,const QJsonValue &)> setter,
             std::function<QStringList(const QString &)> choices,QWidget *parent=nullptr)
        :QTabWidget(parent),read(getter),write(setter),options(choices),resolution(nullptr),fillColor(nullptr),customColor(nullptr),paddingForm(nullptr),audioTrack(nullptr),audioFields(nullptr) {
        setObjectName("easyView");
        auto *summary=page("Summary"); auto *output=group(summary,"Output settings");
        choice(output,"Format","FileFormat",{{"MP4","av_mp4"},{"Matroska (MKV)","av_mkv"},{"WebM","av_webm"}});
        check(output,"Web optimized","Optimize"); check(output,"Align audio/video start","AlignAVStart");
        check(output,"iPod 5G support","Mp4iPodCompatible"); check(output,"Pass through source metadata","MetadataPassthru");
        auto *details=group(summary,"Preset details");
        auto *description=new QPlainTextEdit; description->setMaximumHeight(110); tag(description,"PresetDescription"); details->addRow("Description",description);
        bindings.append([=,this]{ description->setPlainText(value("PresetDescription").toString()); });
        connect(description,&QPlainTextEdit::textChanged,this,[=,this]{ set("PresetDescription",description->toPlainText()); });
        note(summary,"These settings describe a reusable preset. Source scanning, previews, and encoding remain in HandBrake.");

        auto *dimensions=page("Dimensions"); auto *size=group(dimensions,"Resolution and scaling");
        resolution=new QComboBox; resolution->setObjectName("resolutionLimit");
        const QList<QPair<QString,QSize>> limits{{"None / unlimited",QSize(0,0)},{"4320p 8K Ultra HD",QSize(7680,4320)},
            {"2160p 4K Ultra HD",QSize(3840,2160)},{"1080p Full HD",QSize(1920,1080)},{"720p HD",QSize(1280,720)},
            {"576p PAL",QSize(720,576)},{"480p NTSC",QSize(720,480)},{"Custom",QSize(-1,-1)}};
        for(const auto &limit:limits) resolution->addItem(limit.first,limit.second);
        size->addRow("Resolution limit",resolution);
        bindings.append([this]{
            const QSize current(read().value("PictureWidth").toInt(),read().value("PictureHeight").toInt());
            const int index=customResolution?-1:resolution->findData(current);
            resolution->setCurrentIndex(index<0?resolution->count()-1:index);
        });
        connect(resolution,&QComboBox::activated,this,[this](int index){
            const auto limit=resolution->itemData(index).toSize(); customResolution=limit.width()<0;
            if(!customResolution) { set("PictureWidth",limit.width()); set("PictureHeight",limit.height()); refresh(); }
            else dependencies();
        });
        number(size,"Maximum width (0 = unlimited)","PictureWidth"); number(size,"Maximum height (0 = unlimited)","PictureHeight");
        check(size,"Keep aspect ratio","PictureKeepRatio"); check(size,"Use maximum size","PictureUseMaximumSize"); check(size,"Allow upscaling","PictureAllowUpscaling");
        choice(size,"Anamorphic","PicturePAR",{{"Automatic","auto"},{"None","off"},{"Custom","custom"}});
        number(size,"Pixel aspect width","PicturePARWidth"); number(size,"Pixel aspect height","PicturePARHeight");
        auto *crop=group(dimensions,"Cropping");
        choice(crop,"Cropping","PictureCropMode",{{"Automatic",0},{"Loose",1},{"None",2},{"Custom",3}});
        number(crop,"Top","PictureTopCrop"); number(crop,"Bottom","PictureBottomCrop"); number(crop,"Left","PictureLeftCrop"); number(crop,"Right","PictureRightCrop");
        auto *padding=group(dimensions,"Borders and padding"); paddingForm=padding;
        choice(padding,"Fill style","PicturePadMode",{{"None","none"},{"Letterbox · fill height","letterbox"},{"Pillarbox · fill width","pillarbox"},{"Fill · both dimensions","fill"},{"Custom","custom"}});
        fillColor=new QComboBox; fillColor->setObjectName("fillColor"); fillColor->addItem("Black","black"); fillColor->addItem("White","white"); fillColor->addItem("Custom RGB…","custom"); padding->addRow("Fill color",fillColor);
        customColor=new QLineEdit; customColor->setObjectName("customFillColor"); customColor->setPlaceholderText("R, G, B (0–255), e.g. 24, 32, 48"); padding->addRow("Custom RGB",customColor);
        customColor->setValidator(new QRegularExpressionValidator(QRegularExpression(
            R"((\s*(?:25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9]?[0-9])\s*,){2}\s*(?:25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9]?[0-9])\s*|(?:#|0x)?[0-9a-fA-F]{6})"),customColor));
        connect(customColor,&QLineEdit::textEdited,this,[this]{
            customColor->setProperty("invalid",!customColor->hasAcceptableInput()); customColor->style()->unpolish(customColor); customColor->style()->polish(customColor);
        });
        bindings.append([this]{
            const auto color=read().value("PicturePadColor").toString();
            const bool named=color.isEmpty() || color=="black" || color=="white";
            fillColor->setCurrentIndex(named?(color=="white"?1:0):2);
            if(!named) lastCustomColor=color;
            QColor parsed(color);
            if(!parsed.isValid()) parsed=QColor(color.startsWith('#')?color:color.startsWith("0x")?"#"+color.mid(2):"#"+color);
            if(parsed.isValid()) customColor->setText(QString("%1, %2, %3").arg(parsed.red()).arg(parsed.green()).arg(parsed.blue()));
            else customColor->setText(named?QString():color);
            customColor->setProperty("invalid",false);
        });
        connect(fillColor,&QComboBox::activated,this,[this](int index){
            set("PicturePadColor",index==2?lastCustomColor:fillColor->itemData(index).toString()); dependencies();
        });
        connect(customColor,&QLineEdit::editingFinished,this,[this]{
            if(!customColor->isModified()) return;
            const auto text=customColor->text().trimmed(); const auto parts=text.split(','); QColor color;
            if(parts.size()==3) {
                bool valid=true; int rgb[3]; for(int i=0;i<3;++i) { bool ok=false; rgb[i]=parts[i].trimmed().toInt(&ok); valid=valid && ok && rgb[i]>=0 && rgb[i]<=255; }
                if(valid) color=QColor(rgb[0],rgb[1],rgb[2]);
            } else color=QColor(text.startsWith('#')?text:text.startsWith("0x")?"#"+text.mid(2):"#"+text);
            if(!color.isValid()) { customColor->setToolTip("Enter R, G, B values from 0 to 255, or #RRGGBB."); customColor->setProperty("invalid",true); return; }
            customColor->setProperty("invalid",false); customColor->setModified(false); lastCustomColor="0x"+color.name().mid(1); set("PicturePadColor",lastCustomColor);
        });
        for(const auto &edge:{"Top","Bottom","Left","Right"}) number(padding,edge,QString("PicturePad")+edge);

        auto *filters=page("Filters"); auto *interlace=group(filters,"Interlace and film");
        choice(interlace,"Detelecine","PictureDetelecine",{{"Off","off"},{"Default","default"},{"Custom","custom"}});
        line(interlace,"Custom detelecine","PictureDetelecineCustom");
        choice(interlace,"Interlace detection","PictureCombDetectPreset",{{"Off","off"},{"Default","default"}});
        line(interlace,"Custom interlace detection","PictureCombDetectCustom");
        choice(interlace,"Deinterlace","PictureDeinterlaceFilter",{{"Off","off"},{"Decomb","decomb"},{"Yadif","yadif"},{"Bwdif","bwdif"}});
        choice(interlace,"Deinterlace preset","PictureDeinterlacePreset",{{"Default","default"}});
        line(interlace,"Custom deinterlace","PictureDeinterlaceCustom");
        auto *clean=group(filters,"Denoise and sharpen");
        choice(clean,"Denoise","PictureDenoiseFilter",{{"Off","off"},{"HQDN3D","hqdn3d"},{"NLMeans","nlmeans"}});
        choice(clean,"Denoise preset","PictureDenoisePreset"); choice(clean,"Denoise tune","PictureDenoiseTune");
        line(clean,"Custom denoise","PictureDenoiseCustom");
        choice(clean,"Sharpen","PictureSharpenFilter",{{"Off","off"},{"LapSharp","lapsharp"},{"Unsharp","unsharp"}});
        choice(clean,"Sharpen preset","PictureSharpenPreset"); choice(clean,"Sharpen tune","PictureSharpenTune");
        line(clean,"Custom sharpen","PictureSharpenCustom");
        auto *color=group(filters,"Color and detail");
        choice(color,"Chroma smooth","PictureChromaSmoothPreset",{{"Off","off"}}); choice(color,"Chroma smooth tune","PictureChromaSmoothTune"); line(color,"Custom chroma smooth","PictureChromaSmoothCustom");
        choice(color,"Deblock","PictureDeblockPreset",{{"Off","off"}}); choice(color,"Deblock tune","PictureDeblockTune"); line(color,"Custom deblock","PictureDeblockCustom");
        choice(color,"Colorspace","PictureColorspacePreset",{{"Off","off"}}); line(color,"Custom colorspace","PictureColorspaceCustom"); check(color,"Grayscale","VideoGrayScale");
        note(filters,"Hover a filter for an explanation. Preset strength increases from Ultralight to Very strong. Custom reveals the HandBrake key=value expression field.");
        const QMap<QString,QString> filterHelp{
            {"PictureDetelecine","Removes repeated fields introduced when film is converted to telecined video. Default handles typical telecine; Custom accepts a filter expression."},
            {"PictureCombDetectPreset","Detects interlaced/combed frames for selective deinterlacing. Less sensitive reduces detection; Fast uses a faster detector. This detector does not deinterlace by itself."},
            {"PictureDeinterlaceFilter","Decomb selectively deinterlaces combed frames. Yadif and Bwdif reconstruct progressive frames. Bob outputs both fields as separate frames; EEDI2 uses edge-directed interpolation."},
            {"PictureDenoiseFilter","HQDN3D reduces spatial and temporal noise. NLMeans preserves detail using similar image patches and offers content tunes. BM3D uses block matching. Stronger presets remove more noise and may remove detail."},
            {"PictureSharpenFilter","LapSharp uses Laplacian edge sharpening; Unsharp uses an unsharp mask. Stronger presets increase edge contrast; tunes change the spatial scale or content assumptions."},
            {"PictureChromaSmoothPreset","Smooths color noise while retaining luma detail. Strength controls smoothing; tune controls its spatial extent from Tiny to Very wide."},
            {"PictureDeblockPreset","Reduces block artifacts from compression. Strength increases from Ultralight to Very strong. Tune selects 4x4, 8x8, or 16x16 block size."},
            {"PictureColorspacePreset","Converts color primaries, transfer, and matrix to BT.2020, BT.709, or BT.601. HandBrake applies its preset tone-mapping settings where needed; Custom exposes the conversion expression."},
            {"VideoGrayScale","Removes color from the output video."}};
        for(auto it=filterHelp.begin();it!=filterHelp.end();++it) controls[it.key()]->setToolTip(it.value());
        const QMap<QString,QString> families{{"PictureDetelecine","PictureDetelecine"},{"PictureCombDetect","PictureCombDetectPreset"},
            {"PictureDeinterlace","PictureDeinterlaceFilter"},{"PictureDenoise","PictureDenoiseFilter"},{"PictureSharpen","PictureSharpenFilter"},
            {"PictureChromaSmooth","PictureChromaSmoothPreset"},{"PictureDeblock","PictureDeblockPreset"},{"PictureColorspace","PictureColorspacePreset"}};
        for(auto it=controls.begin();it!=controls.end();++it)
            for(auto family=families.begin();family!=families.end();++family)
                if(it.key().startsWith(family.key())) it.value()->setToolTip(filterHelp.value(family.value())+"\n\n"+it.key()+" · Custom accepts a colon-separated key=value expression.");

        auto *video=page("Video"); auto *codec=group(video,"Video encoder");
        choice(codec,"Encoder","VideoEncoder"); choice(codec,"Framerate (FPS)","VideoFramerate",{{"Same as source","auto"},{"23.976","23.976"},{"24","24"},{"25","25"},{"29.97","29.97"},{"30","30"},{"50","50"},{"59.94","59.94"},{"60","60"}});
        choice(codec,"Framerate mode","VideoFramerateMode",{{"Variable framerate (VFR)","vfr"},{"Constant framerate (CFR)","cfr"}});
        auto *quality=group(video,"Quality");
        choice(quality,"Rate control","VideoQualityType",{{"Constant quality",2},{"Average bitrate",1}});
        number(quality,"Constant quality (encoder scale)","VideoQualitySlider",0); number(quality,"Average bitrate (kbps)","VideoAvgBitrate");
        check(quality,"Multi-pass encoding","VideoMultiPass"); check(quality,"Turbo first pass","VideoTurboMultiPass");
        auto *encoder=group(video,"Encoder options"); choice(encoder,"Encoder preset","VideoPreset");
        choice(encoder,"Tune","VideoTune"); choice(encoder,"Profile","VideoProfile"); choice(encoder,"Level","VideoLevel");
        line(encoder,"Extra arguments","VideoOptionExtra");
        note(video,"Encoder options are gathered from your library. Availability depends on the encoder and the HandBrake installation.");

        auto *audio=page("Audio"); auto *selection=group(audio,"Track selection defaults");
        choice(selection,"Track selection","AudioTrackSelectionBehavior",{{"First matching track","first"},{"All matching tracks","all"},{"None","none"}});
        line(selection,"Preferred language codes","AudioLanguageList",true); check(selection,"Keep source track names","AudioTrackNamePassthru");
        check(selection,"Secondary encoders for first track only","AudioSecondaryEncoderMode");
        choice(selection,"Fallback encoder","AudioEncoderFallback"); line(selection,"Allowed passthrough encoders","AudioCopyMask",true);
        auto *trackBox=new QGroupBox("Audio encoder templates"); auto *trackLayout=new QVBoxLayout(trackBox); auto *trackBar=new QHBoxLayout;
        audioTrack=new QComboBox; audioTrack->setObjectName("audioTrackPicker"); trackBar->addWidget(audioTrack,1);
        auto *add=new QPushButton("Add / duplicate"); auto *remove=new QPushButton("Remove"); trackBar->addWidget(add); trackBar->addWidget(remove); trackLayout->addLayout(trackBar);
        audioFields=new QWidget; auto *trackForm=new QFormLayout(audioFields); trackForm->setSpacing(12); trackLayout->addWidget(audioFields); audio->addWidget(trackBox);
        choice(trackForm,"Encoder","AudioEncoder",{},true); number(trackForm,"Bitrate (kbps)","AudioBitrate",0,true);
        choice(trackForm,"Mixdown","AudioMixdown",{},true); choice(trackForm,"Sample rate","AudioSamplerate",{{"Automatic","auto"}},true);
        number(trackForm,"Gain (dB)","AudioTrackGainSlider",2,true); number(trackForm,"Dynamic range compression","AudioTrackDRCSlider",2,true);
        check(trackForm,"Normalize mix level","AudioNormalizeMixLevel",true); check(trackForm,"Use quality instead of bitrate","AudioTrackQualityEnable",true);
        number(trackForm,"Audio quality","AudioTrackQuality",3,true);
        connect(audioTrack,&QComboBox::activated,this,[this](int index){ trackIndex=index; refresh(); });
        connect(add,&QPushButton::clicked,this,[this]{
            auto array=read().value("AudioList").toArray();
            const auto object=array.isEmpty()?audioTemplate:array[std::clamp(trackIndex,0,int(array.size())-1)].toObject();
            if(object.isEmpty()) return;
            array.append(object); write("AudioList",array); trackIndex=int(array.size())-1; refresh();
        });
        connect(remove,&QPushButton::clicked,this,[this]{
            auto array=read().value("AudioList").toArray(); if(trackIndex<0 || trackIndex>=array.size()) return;
            array.removeAt(trackIndex); write("AudioList",array); refresh();
        });
        bindings.append([=,this]{ add->setEnabled(!read().value("AudioList").toArray().isEmpty() || !audioTemplate.isEmpty()); remove->setEnabled(!read().value("AudioList").toArray().isEmpty()); });
        note(audio,"These templates define how audio tracks are selected and encoded when HandBrake scans a source.");

        auto *subtitles=page("Subtitles"); auto *sub=group(subtitles,"Subtitle selection defaults");
        choice(sub,"Track selection","SubtitleTrackSelectionBehavior",{{"None","none"},{"First matching track","first"},{"All matching tracks","all"}});
        line(sub,"Preferred language codes","SubtitleLanguageList",true); check(sub,"Keep source track names","SubtitleTrackNamePassthru");
        check(sub,"Foreign audio search","SubtitleAddForeignAudioSearch"); check(sub,"Add foreign audio subtitle","SubtitleAddForeignAudioSubtitle"); check(sub,"Include closed captions","SubtitleAddCC");
        auto *burn=group(subtitles,"Burn-in behavior"); choice(burn,"Burn subtitles","SubtitleBurnBehavior",{{"None","none"},{"First subtitle","first"},{"Foreign audio","foreign"},{"Foreign audio, then first","foreign_first"}});
        check(burn,"Burn DVD subtitles","SubtitleBurnDVDSub"); check(burn,"Burn Blu-ray subtitles","SubtitleBurnBDSub");
        auto *chapters=page("Chapters"); auto *chapter=group(chapters,"Chapter markers"); check(chapter,"Include chapter markers","ChapterMarkers");
        note(chapters,"Chapter names and timestamps come from the source and are edited in HandBrake after scanning. The preset stores whether to include markers.");
        setEnabled(false);
    }
    void setAudioTemplate(const QJsonObject &object) { audioTemplate=object; }
    void refresh() {
        syncing=true; setUpdatesEnabled(false);
        const auto array=read().value("AudioList").toArray(); trackIndex=std::clamp(trackIndex,0,std::max(0,int(array.size())-1));
        audioTrack->clear();
        for(int i=0;i<array.size();++i) audioTrack->addItem(QString("Template %1 · %2").arg(i+1).arg(array[i].toObject().value("AudioEncoder").toString()));
        audioTrack->setCurrentIndex(array.isEmpty()?-1:trackIndex);
        for(const auto &binding:bindings) binding();
        dependencies(); syncing=false; setUpdatesEnabled(true);
    }
    void setEncoderOptions(const QMap<QString,QMap<QString,QStringList>> &values) { encoderOptions=values; }
    QString validationError() const {
        return !customColor->isHidden() && customColor->isEnabled() && (customColor->isModified() || customColor->property("invalid").toBool()) && !customColor->hasAcceptableInput()
            ?QString("Enter a valid custom fill color: R, G, B values from 0 to 255, or #RRGGBB."):QString();
    }
    void resetTrack() { trackIndex=0; customResolution=false; lastCustomColor="000000"; }
};
