#pragma once

#include <QString>
#include <QDir>
#include <QBitArray>
#include <QVector>
#include <iostream>
#include <SFML/Graphics.hpp>
#include <memory>

#include "Jazz2FormatDataBlock.h"

enum Jazz2TilesetVersion {
    BASE_GAME,
    TSF
};

struct Jazz2TilesetTile {
    bool opaque;
    quint32 imageDataOffset;
    quint32 alphaDataOffset;
    quint32 maskDataOffset;
    sf::Image image;
    sf::Image mask;
};

class Jazz2Tileset {
public:
    Jazz2Tileset();
    static Jazz2Tileset* fromFile(const QString& filename, bool strictParser = false);
    void saveAsProjectCarrotTileset(const QDir& directory, const QString& uniqueID);
    void printData(std::ostream& target);

private:
    QString name;
    Jazz2TilesetVersion version;
    QVector<sf::Color> palette;
    QVector<std::shared_ptr<Jazz2TilesetTile>> tiles;
    quint32 tileCount;

    // Parser utility functions
    Jazz2Tileset(Jazz2FormatDataBlock& header, quint32 fileLength, bool strictParser);
    void loadMetadata(Jazz2FormatDataBlock& block, bool strictParser);
    void loadImageData(Jazz2FormatDataBlock& imageBlock, Jazz2FormatDataBlock& alphaBlock, bool strictParser);
    void loadMaskData(Jazz2FormatDataBlock& block, bool strictParser);

    void writePCConfigFile(const QString& filename, const QString& uniqueID);

    int maxSupportedTiles();
};