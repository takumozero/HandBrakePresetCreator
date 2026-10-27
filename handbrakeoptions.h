#pragma once
#include <QJsonObject>
#include <QStringList>

// Parameter names and enumerated values checked against HandBrake master,
// libhb/param.c, encavcodec.c, and handbrake/{h264,h265,av1}_common.h.
// These are capability data; no upstream implementation is incorporated.
namespace HandBrakeOptions {
inline QStringList split(const char *text) { return QString::fromLatin1(text).split(',',Qt::KeepEmptyParts); }
inline QStringList filter(const QString &key,const QJsonObject &p) {
    if(key=="PictureDetelecine") return split("off,default,custom");
    if(key=="PictureCombDetectPreset") return split("off,default,permissive,fast,custom");
    if(key=="PictureDeinterlaceFilter") return split("off,decomb,yadif,bwdif");
    if(key=="PictureDeinterlacePreset") {
        const auto f=p.value("PictureDeinterlaceFilter").toString();
        if(f=="decomb") return split("default,bob,eedi2,eedi2bob,custom");
        if(f=="yadif") return split("default,skip-spatial,bob,custom");
        return split("default,bob,custom");
    }
    if(key=="PictureDenoiseFilter") return split("off,hqdn3d,nlmeans,bm3d");
    if(key=="PictureDenoisePreset") return p.value("PictureDenoiseFilter")=="bm3d"?split("default,medium,strong,custom"):split("ultralight,light,medium,strong,custom");
    if(key=="PictureDenoiseTune") return split("none,film,grain,highmotion,animation,tape,sprite");
    if(key=="PictureSharpenFilter") return split("off,lapsharp,unsharp");
    if(key=="PictureSharpenPreset" || key=="PictureChromaSmoothPreset" || key=="PictureDeblockPreset") {
        auto list=split("ultralight,light,medium,strong,stronger,verystrong,custom");
        if(key!="PictureSharpenPreset") list.prepend("off"); return list;
    }
    if(key=="PictureSharpenTune") return p.value("PictureSharpenFilter")=="unsharp"?split("none,ultrafine,fine,medium,coarse,verycoarse"):split("none,film,grain,animation,sprite");
    if(key=="PictureChromaSmoothTune") return split("none,tiny,small,medium,wide,verywide");
    if(key=="PictureDeblockTune") return split("small,medium,large");
    if(key=="PictureColorspacePreset") return split("off,bt2020,bt709,bt601-6-525,bt601-6-625,custom");
    return {};
}
inline QStringList encoder(const QString &key,const QString &enc) {
    const bool x264=enc.startsWith("x264"),x265=enc.startsWith("x265"),nv=enc.startsWith("nvenc"),svt=enc.startsWith("svt_av1");
    const bool h264=x264 || enc.contains("h264"),hevc=x265 || enc.contains("h265"),ten=enc.contains("10bit");
    if(key=="VideoPreset") {
        if(x264 || x265) return split("ultrafast,superfast,veryfast,faster,fast,medium,slow,slower,veryslow,placebo");
        if(nv) return split("fastest,faster,fast,medium,slow,slower,slowest");
        if(svt) return split("10,9,8,7,6,5,4,3,2,1,0,-1");
    }
    if(key=="VideoTune") {
        if(x264) return split(",film,animation,grain,stillimage,psnr,ssim,fastdecode,zerolatency");
        if(x265) return split(",psnr,ssim,grain,zerolatency,fastdecode,animation");
        if(nv) return split("");
        if(svt) return split("vq,psnr,ssim,iq,ms-ssim,vmaf,fastdecode");
    }
    if(key=="VideoProfile") {
        if(x264) return ten?split("auto,high10,high422,high444"):split("auto,baseline,main,high,high422,high444");
        if(x265) {
            if(ten) return split("auto,main10,main10-intra,main422-10,main422-10-intra,main444-10,main444-10-intra");
            if(enc.contains("12bit")) return split("auto,main12,main12-intra,main422-12,main422-12-intra,main444-12,main444-12-intra");
            return split("auto,main,mainstillpicture,main444-8,main444-intra");
        }
        if(nv && h264) return ten?split("auto,high10"):split("auto,baseline,main,high,high444p");
        if(nv && hevc) return ten?split("auto,main10"):split("auto,main");
        if(nv) return split("auto");
        if(svt) return split("auto,main,high,profesional");
    }
    if(key=="VideoLevel") {
        if(h264) return split("auto,1.0,1b,1.1,1.2,1.3,2.0,2.1,2.2,3.0,3.1,3.2,4.0,4.1,4.2,5.0,5.1,5.2,6.0,6.1,6.2");
        if(hevc) return split("auto,1.0,2.0,2.1,3.0,3.1,4.0,4.1,5.0,5.1,5.2,6.0,6.1,6.2");
        if(svt) return split("auto,2.0,2.1,2.2,2.3,3.0,3.1,3.2,3.3,4.0,4.1,4.2,4.3,5.0,5.1,5.2,5.3,6.0,6.1,6.2,6.3");
        if(nv) return split("auto");
    }
    return {};
}
inline bool multipass(const QString &enc,bool constantQuality) {
    for(const auto &prefix:{"nvenc","qsv","vce","vcn","mf","dnxhr"}) if(enc.startsWith(QLatin1String(prefix))) return false;
    if(enc.startsWith("vp9") || enc.startsWith("ffv1")) return true;
    return !constantQuality;
}
}
