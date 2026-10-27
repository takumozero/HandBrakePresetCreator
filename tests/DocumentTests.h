#pragma once
#include "../presetdocument.h"

static int selfTest() {
    const QByteArray input = R"({"VersionMajor":60,"unknown":"keep","PresetList":[{"Folder":true,"PresetName":"Folder","ChildrenArray":[{"PresetName":"A","VideoEncoder":"x264","VideoQualityType":2,"VideoQualitySlider":20,"AudioList":[{"AudioEncoder":"aac"}],"future":42},{"PresetName":"B","VideoEncoder":"x265"}]}]})";
    PresetDocument d; QString error;
    if (!d.load(input,error) || d.presets().size()!=2) return 1;
    auto p = d.presets()[0]; p["PresetName"]="Edited"; p["VideoQualitySlider"]=22.5; d.replace(0,p);
    if (d.presets()[0]["future"].toInt()!=42 || d.presets()[1]["PresetName"]!="B"
        || d.json.object()["unknown"]!="keep" || d.presets()[0]["AudioList"].toArray().size()!=1) return 2;
    PresetDocument roundTrip;
    if (!roundTrip.load(d.json.toJson(),error) || roundTrip.json!=d.json) return 3;
    const auto before=d.json;
    if (d.load("invalid",error) || d.json!=before || d.load("{}",error)) return 4;
    const auto originalFolder = d.json.object()["PresetList"].toArray()[0];
    if (d.addFrom(0)!=2 || d.presets().size()!=3
        || d.json.object()["PresetList"].toArray()[0]!=originalFolder
        || d.json.object()["VersionMajor"].toInt()!=60
        || d.json.object()["unknown"]!="keep"
        || d.presets()[2]["future"].toInt()!=42
        || d.presets()[2]["Type"].toInt()!=1 || d.presets()[2]["Default"].toBool()) return 10;
    d.addFrom(0);
    if (d.presets()[2]["PresetName"]==d.presets()[3]["PresetName"]) return 11;
    d.createFrom(0);
    if (d.presets().size()!=1 || d.json.object()["VersionMajor"].toInt()!=60
        || d.presets()[0]["Type"].toInt()!=1 || d.presets()[0]["Default"].toBool()) return 5;
    if (!d.load(R"([{"PresetName":"Array","VideoEncoder":"x264"}])",error)) return 6;
    p=d.presets()[0]; p["PresetDescription"]="Updated"; d.replace(0,p);
    if (!d.json.isArray() || d.presets()[0]["PresetDescription"]!="Updated") return 7;
    if (d.addFrom(0)!=1 || !d.json.isArray() || d.presets().size()!=2) return 12;
    if (!d.load(R"({"PresetName":"Single","VideoEncoder":"x264"})",error)) return 8;
    if (d.addFrom(0)!=1 || d.presets().size()!=2 || d.presets()[0]["PresetName"]!="Single") return 13;
    d.createFrom(0);
    return d.presets().size()==1 && d.presets()[0]["PresetName"]=="Single (copy) 2" ? 0 : 9;
}

static int deletionTest() {
    PresetDocument d; QString error;
    if(!d.load(R"({"VersionMajor":60,"unknown":"keep","PresetList":[{"Folder":true,"PresetName":"Folder","extra":42,"ChildrenArray":[{"PresetName":"Same","VideoEncoder":"x264","id":1},{"PresetName":"Same","VideoEncoder":"x265","id":2}]},{"PresetName":"Outside","VideoEncoder":"x264"}]})",error)) return 101;
    const auto original=d.json;
    if(d.remove(-1) || d.remove(99) || d.json!=original) return 102;
    if(!d.remove(1) || d.presets().size()!=2 || d.presets()[0]["id"].toInt()!=1 || d.presets()[1]["PresetName"]!="Outside") return 103;
    const auto root=d.json.object(); const auto folder=root["PresetList"].toArray()[0].toObject();
    if(root["VersionMajor"].toInt()!=60 || root["unknown"]!="keep" || folder["extra"].toInt()!=42 || folder["ChildrenArray"].toArray().size()!=1) return 104;
    if(!d.remove(0) || d.json.object()["PresetList"].toArray()[0].toObject()["ChildrenArray"].toArray().size()!=0) return 105;
    if(!d.remove(0) || !d.presets().isEmpty()) return 106;
    PresetDocument reopened; if(!reopened.load(d.json.toJson(),error) || reopened.json!=d.json) return 107;
    if(d.append(QJsonObject{{"PresetName","Replacement"},{"VideoEncoder","x264"}})!=0
        || d.json.object()["unknown"]!="keep" || d.presets().size()!=1) return 112;
    if(!d.load(R"([{"PresetName":"Array","VideoEncoder":"x264"}])",error) || !d.remove(0) || !d.json.isArray() || !d.json.array().isEmpty()) return 108;
    if(!reopened.load(d.json.toJson(),error)) return 109;
    if(!d.load(R"({"PresetName":"Single","VideoEncoder":"x264"})",error) || !d.remove(0) || !reopened.load(d.json.toJson(),error)) return 110;
    return d.presets().isEmpty()?0:111;
}
