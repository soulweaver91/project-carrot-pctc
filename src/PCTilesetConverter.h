#pragma once

#include <QString>
#include <QDir>

class PCTilesetConverter {
public:
    static void convert(const QString& filename, const QString& id, const QDir& outputDir);
};
