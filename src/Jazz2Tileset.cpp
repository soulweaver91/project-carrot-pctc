#include "Jazz2Tileset.h"

#include <QFile>
#include <QSettings>
#include "Jazz2FormatParseException.h"
#include "Version.h"

Jazz2Tileset::Jazz2Tileset() {
}

Jazz2Tileset::Jazz2Tileset(Jazz2FormatDataBlock& header, quint32 fileLength, bool strictParser) {
    // Skip the copyright info at the beginning of the file
    header.discardBytes(180);

    // Read the next four bytes; should spell out "TILE"
    qint32 id = header.readUInt();
    if (id != 0x454C4954) {
        throw Jazz2FormatParseException(INVALID_MAGIC, { ".J2T" });
    }

    quint32 hash = header.readUInt();
    if (hash != 0xAFBEADDE) {
        throw Jazz2FormatParseException(INVALID_MAGIC, { ".J2T" });
    }

    name = header.readRawBytes(32);

    quint16 versionNum = header.readUShort();
    version = (versionNum <= 257 ? BASE_GAME : TSF);

    quint32 recordedSize = header.readUInt();
    if (strictParser && fileLength != recordedSize) {
        throw Jazz2FormatParseException(UNEXPECTED_FILE_LENGTH);
    }

    // Get the CRC; would check here if it matches if we knew what variant it is AND what it applies to
    // Test file across all CRC32 variants + Adler had no matches to the value obtained from the file
    // so either the variant is something else or the CRC is not applied to the whole file but on a part
    quint32 recordedCRC = header.readUInt();
}

Jazz2Tileset* Jazz2Tileset::fromFile(const QString& filename, bool strictParser) {
    QFile fh(filename);
    if (!(fh.exists())) {
        throw Jazz2FormatParseException(FILE_NOT_FOUND, { filename });
    }

    if (!(fh.open(QIODevice::ReadOnly))) {
        throw Jazz2FormatParseException(FILE_CANNOT_BE_OPENED, { filename });
    }

    Jazz2FormatDataBlock headerBlock(fh.read(262), false, 262);
    auto tileset = new Jazz2Tileset(headerBlock, fh.size(), false);

    // Read the lengths, uncompress the blocks and bail if any block could not be uncompressed
    // This could look better without all the copy-paste, but meh.
    quint32 infoBlockPackedSize,   imageBlockPackedSize,   alphaBlockPackedSize,   maskBlockPackedSize,
            infoBlockUnpackedSize, imageBlockUnpackedSize, alphaBlockUnpackedSize, maskBlockUnpackedSize;
    infoBlockPackedSize     = headerBlock.readUInt();
    infoBlockUnpackedSize   = headerBlock.readUInt();
    imageBlockPackedSize    = headerBlock.readUInt();
    imageBlockUnpackedSize  = headerBlock.readUInt();
    alphaBlockPackedSize    = headerBlock.readUInt();
    alphaBlockUnpackedSize  = headerBlock.readUInt();
    maskBlockPackedSize     = headerBlock.readUInt();
    maskBlockUnpackedSize   = headerBlock.readUInt();

    Jazz2FormatDataBlock infoBlock (fh.read(infoBlockPackedSize ), true, infoBlockUnpackedSize  , 0);
    Jazz2FormatDataBlock imageBlock(fh.read(imageBlockPackedSize), true, imageBlockUnpackedSize , 1);
    Jazz2FormatDataBlock alphaBlock(fh.read(alphaBlockPackedSize), true, alphaBlockUnpackedSize , 2);
    Jazz2FormatDataBlock maskBlock (fh.read(maskBlockPackedSize ), true, maskBlockUnpackedSize  , 3);

    // All data is in the memory, so the file can be closed.
    fh.close();

    tileset->loadMetadata(infoBlock, strictParser);
    tileset->loadImageData(imageBlock, alphaBlock, strictParser);
    tileset->loadMaskData(maskBlock, strictParser);

    return tileset;
}

void Jazz2Tileset::saveAsProjectCarrotTileset(const QDir& directory, const QString& uniqueID) {
    writePCConfigFile(directory.absoluteFilePath("config.ini"), uniqueID);
    std::string tilefile = directory.absoluteFilePath("tiles.png").toStdString();
    std::string maskfile = directory.absoluteFilePath("mask.png").toStdString();

    sf::RenderTexture tilesTexture;
    sf::RenderTexture masksTexture;
    tilesTexture.create(320, tileCount / 10 * 32);
    tilesTexture.clear(sf::Color(0, 0, 0, 0));
    masksTexture.create(320, tileCount / 10 * 32);
    masksTexture.clear(sf::Color(0, 0, 0, 0));
        
    sf::Texture singleTileTexture;
    sf::Sprite singleTileSprite;

    singleTileTexture.create(32, 32);
    singleTileSprite.setTexture(singleTileTexture, true);

    for (int i = 0; i < tileCount; ++i) {
        singleTileTexture.update(tiles.at(i)->image);
        singleTileSprite.setPosition((i % 10) * 32, (i / 10) * 32);

        tilesTexture.draw(singleTileSprite);

        singleTileTexture.update(tiles.at(i)->mask);
        masksTexture.draw(singleTileSprite);
    }

    tilesTexture.display();
    masksTexture.display();

    sf::Image finalImageSurface = tilesTexture.getTexture().copyToImage();
    finalImageSurface.saveToFile(tilefile);

    finalImageSurface = masksTexture.getTexture().copyToImage();
    finalImageSurface.createMaskFromColor(sf::Color::White, 0);
    finalImageSurface.saveToFile(maskfile);
}

void Jazz2Tileset::printData(std::ostream& target) {
    target << "Tileset name: " << name.toStdString()        << "\n"
           << " JJ2 version: " << (version == BASE_GAME ? "Base game" : "TSF/CC") << "\n"
           << "       Tiles: " << tileCount
           <<      " (out of " << maxSupportedTiles() / 10 << "0 supported by version)\n\n";

    int i = 0;
    target << "Palette:";
    for (auto c : palette) {
        if ((i++ % 8) == 0) {
            target << "\n  " << QString::number(i).rightJustified(3).toStdString()
                   << "-"    << QString::number(i + 7).rightJustified(3).toStdString()
                   << ": ";
        } else {
            target << ", ";
        }
        target << "#"
               << QString::number(c.r, 16).rightJustified(2, '0').toStdString()
               << QString::number(c.g, 16).rightJustified(2, '0').toStdString()
               << QString::number(c.b, 16).rightJustified(2, '0').toStdString();
    }
    target << "\n";
}

void Jazz2Tileset::loadMetadata(Jazz2FormatDataBlock& block, bool strictParser) {
    palette.clear();
    palette.resize(256);
    for (auto& c : palette) {
        sf::Uint8 red   = block.readChar();
        sf::Uint8 green = block.readChar();
        sf::Uint8 blue  = block.readChar();
        sf::Uint8 alpha = block.readChar();
        c = sf::Color(red, green, blue, 255 - alpha);
    }

    tileCount = block.readUInt();

    int maxTiles = maxSupportedTiles();

    for (int i = 0; i < maxTiles; ++i) {
        bool opaque = (block.readBool());
        auto tile = std::make_shared<Jazz2TilesetTile>();
        tile->opaque = opaque;
        tiles << tile;
    }

    // block of unknown values, skip
    block.discardBytes(maxTiles);

    for (int i = 0; i < maxTiles; ++i) {
        tiles.at(i)->imageDataOffset = block.readUInt();
    }

    // block of unknown values, skip
    block.discardBytes(4 * maxTiles);

    for (int i = 0; i < maxTiles; ++i) {
        tiles.at(i)->alphaDataOffset = block.readUInt();
    }

    // block of unknown values, skip
    block.discardBytes(4 * maxTiles);

    for (int i = 0; i < maxTiles; ++i) {
        tiles.at(i)->maskDataOffset = block.readUInt();
    }

    // we don't care about the flipped masks, those are generated on runtime
    block.discardBytes(4 * maxTiles);
}

void Jazz2Tileset::loadImageData(Jazz2FormatDataBlock& imageBlock, Jazz2FormatDataBlock& alphaBlock, bool strictParser) {
    for (auto tile : tiles) {
        tile->image.create(32, 32);

        QByteArray imageData     = imageBlock.readRawBytes(1024, tile->imageDataOffset);
        QByteArray alphaMaskData = alphaBlock.readRawBytes(128,  tile->alphaDataOffset);
        for (int i = 0; i < 1024; ++i) {
            unsigned char idx = static_cast<unsigned char>(imageData.at(i));
            auto color = palette.at(idx);
            if (((alphaMaskData.at(i / 8) >> (i % 8)) & 0x01) == 0x00) {
                color = sf::Color::Transparent;
            }
            tile->image.setPixel(i % 32, i / 32, color);
        }
    }
}

void Jazz2Tileset::loadMaskData(Jazz2FormatDataBlock& block, bool strictParser) {
    for (auto tile : tiles) {
        tile->mask.create(32, 32);

        QByteArray maskData = block.readRawBytes(128, tile->maskDataOffset);
        for (int i = 0; i < 128; ++i) {
            unsigned char byte = maskData.at(i);
            for (int j = 0; j < 8; ++j) {
                uint pixelIdx = 8 * i + j;
                if (((byte >> j) & 0x01) == 0) {
                    tile->mask.setPixel(pixelIdx % 32, pixelIdx / 32, sf::Color::Transparent);
                }
            }
        }
    }
}

void Jazz2Tileset::writePCConfigFile(const QString& filename, const QString& uniqueID) {
    QSettings file(filename, QSettings::IniFormat);
    if (!file.isWritable()) {
        throw Jazz2FormatParseException(FILE_CANNOT_BE_OPENED, { filename });
    }

    file.beginGroup("Version");
    file.setValue("WritingApp", "PCTC-" + QString(CONVERTERVERSION));
    file.endGroup();

    file.beginGroup("Tileset");
    file.setValue("TilesetToken", uniqueID);
    file.setValue("FormalName", name);

    file.sync();
}

int Jazz2Tileset::maxSupportedTiles() {
    return (version == BASE_GAME) ? 1024 : 4096;
}
