#pragma once
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <functional>

class PresetDocument {
public:
    QJsonDocument json;
    QList<QJsonObject> presets() const {
        QList<QJsonObject> result;
        collect(json.isArray() ? QJsonValue(json.array()) : QJsonValue(json.object()), result);
        return result;
    }
    bool load(const QByteArray &bytes, QString &error) {
        QJsonParseError parse;
        auto candidate = QJsonDocument::fromJson(bytes, &parse);
        if (parse.error != QJsonParseError::NoError) {
            error = parse.errorString(); return false;
        }
        PresetDocument check; check.json = candidate;
        const auto emptyEntries=candidate.isArray()?candidate.array():candidate.object().value("PresetList").toArray();
        const bool library=candidate.isArray() || candidate.object().value("PresetList").isArray();
        if (check.presets().isEmpty() && !(library && emptyFolders(emptyEntries))) {
            error = "No HandBrake presets were found in this JSON file."; return false;
        }
        json = candidate; return true;
    }
    void replace(int index, const QJsonObject &preset) {
        int current = 0;
        auto value = walk(json.isArray() ? QJsonValue(json.array()) : QJsonValue(json.object()),
                         [&](QJsonObject &p) { if (current++ == index) p = preset; });
        json = value.isArray() ? QJsonDocument(value.toArray()) : QJsonDocument(value.toObject());
    }
    void createFrom(int index) {
        auto p = copyOf(index);
        // Keep the source export's version metadata, and only the selected preset.
        if (json.isObject() && json.object().contains("PresetList")) {
            auto root = json.object(); root["PresetList"] = QJsonArray{p}; json = QJsonDocument(root);
        } else if (json.isArray()) json = QJsonDocument(QJsonArray{p});
        else json = QJsonDocument(p);
    }
    int addFrom(int index) {
        const auto entries = presets();
        if (index < 0 || index >= entries.size()) return -1;
        return append(copyOf(index));
    }
    int append(const QJsonObject &p) {
        if(!p.value("PresetName").isString() || !p.value("VideoEncoder").isString() || p.value("Folder").toBool()) return -1;
        const int index=presets().size();
        if (json.isObject() && json.object().value("PresetList").isArray()) {
            auto root = json.object(); auto array = root["PresetList"].toArray();
            array.append(p); root["PresetList"] = array; json = QJsonDocument(root);
        } else if (json.isArray()) {
            auto array = json.array(); array.append(p); json = QJsonDocument(array);
        } else {
            // A standalone preset becomes a library without losing the original.
            json = QJsonDocument(QJsonObject{{"PresetList", QJsonArray{json.object(), p}}});
        }
        return index;
    }
    bool remove(int index) {
        if(index<0 || index>=presets().size()) return false;
        int current=0;
        const auto value=removeEntry(json.isArray()?QJsonValue(json.array()):QJsonValue(json.object()),index,current);
        if(value.isUndefined()) json=QJsonDocument(QJsonObject{{"PresetList",QJsonArray{}}});
        else json=value.isArray()?QJsonDocument(value.toArray()):QJsonDocument(value.toObject());
        return true;
    }
private:
    static bool emptyFolders(const QJsonArray &array) {
        for(const auto &value:array) {
            const auto object=value.toObject();
            if(!value.isObject() || !object.value("Folder").toBool() || !object.value("ChildrenArray").isArray()
                || !emptyFolders(object.value("ChildrenArray").toArray())) return false;
        }
        return true;
    }
    static QJsonValue removeEntry(const QJsonValue &value,int index,int &current) {
        if(value.isArray()) {
            QJsonArray result;
            for(const auto &entry:value.toArray()) {
                const auto next=removeEntry(entry,index,current); if(!next.isUndefined()) result.append(next);
            }
            return result;
        }
        if(!value.isObject()) return value;
        auto object=value.toObject();
        if(object.value("PresetName").isString() && object.value("VideoEncoder").isString() && !object.value("Folder").toBool())
            return current++==index?QJsonValue(QJsonValue::Undefined):value;
        for(const auto &key:{QStringLiteral("PresetList"),QStringLiteral("ChildrenArray")})
            if(object.value(key).isArray()) object[key]=removeEntry(object.value(key),index,current);
        return object;
    }
    static void collect(const QJsonValue &value,QList<QJsonObject> &result) {
        if(value.isArray()) {
            for(const auto &entry:value.toArray()) collect(entry,result);
        } else if(value.isObject()) {
            const auto object=value.toObject();
            if(object.value("PresetName").isString() && object.value("VideoEncoder").isString()
                && !object.value("Folder").toBool()) result.append(object);
            else for(const auto &key:{QStringLiteral("PresetList"),QStringLiteral("ChildrenArray")})
                if(object.value(key).isArray()) collect(object.value(key),result);
        }
    }
    QJsonObject copyOf(int index) const {
        const auto entries = presets();
        auto p = entries.at(index);
        const auto base = p["PresetName"].toString() + " (copy)";
        QString candidate = base;
        int suffix = 2;
        auto exists = [&](const QString &name) {
            for (const auto &entry : entries)
                if (entry["PresetName"].toString().compare(name, Qt::CaseInsensitive) == 0) return true;
            return false;
        };
        while (exists(candidate)) candidate = base + " " + QString::number(suffix++);
        p["PresetName"] = candidate; p["Type"] = 1; p["Default"] = false;
        return p;
    }
    static QJsonValue walk(QJsonValue value, const std::function<void(QJsonObject &)> &visit) {
        if (value.isArray()) {
            auto array = value.toArray();
            for (int i = 0; i < array.size(); ++i) array[i] = walk(array[i], visit);
            return array;
        }
        if (!value.isObject()) return value;
        auto object = value.toObject();
        if (object.value("PresetName").isString() && object.value("VideoEncoder").isString()
            && !object.value("Folder").toBool()) visit(object);
        else for (const auto &key : {QStringLiteral("PresetList"), QStringLiteral("ChildrenArray")})
            if (object.value(key).isArray()) object[key] = walk(object[key], visit);
        return object;
    }
};
