// J2T to native Project Carrot tileset file converter
// (c) 2013-2017 Soulweaver

#define CONVERTERVERSION "1.0.1"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <QByteArray>
#include <QBitArray>
#include <QString>
#include <QFile>
#include <QDir>
#include <QVector>
#include <iostream>
#include <cassert>

#include "Jazz2FormatDataBlock.h"
#include "Jazz2FormatParseException.h"

struct J2Tile {
    bool opaque;
    quint32 imageIdx;
    quint32 alphaIdx;
    quint32 maskIdx;
    QBitArray alphaMask;
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cout << "PCTC version " << CONVERTERVERSION << "\n"
                  << "Usage: pctc name inputfile\n\n"
                  << "Converts a Jazz Jackrabbit 2 tileset file (.j2t) to the native, multiple file\n"
                  << "based format recognized by Project Carrot.\n\n"
                  << "Parameters:\n"
                  << "  name         Required. The unique identifying name of this tileset. Use only\n"
                  << "               Latin letters, numbers and underscores. For the purposes of\n"
                  << "               being cross platform compliant, identifiers are case\n"
                  << "               insensitive.\n"
                  << "  inputfile    Required. Complete or relative path to a J2T format file to be\n"
                  << "               converted.\n";
        return EXIT_FAILURE;
    }

    QString unique_id(argv[1]);
    QString filename;
    for (int i = 2; i < argc; ++i) {
        filename += argv[i];
    }

    QFile fh(filename);
    if (!(fh.exists())) {
        std::cerr << "ERROR: The input file \"" << filename.toLocal8Bit().data() << "\" cannot be found!\n";
        return EXIT_FAILURE;
    }
    if (!(fh.open(QIODevice::ReadOnly))) {
        std::cerr << "ERROR: Cannot open input file \"" << filename.toLocal8Bit().data() << "\"!\n";
        return EXIT_FAILURE;
    }
    std::cout << "Opening input file \"" << filename.toLocal8Bit().data() << "\"...\n";

    qint32 filesize = fh.size();

    try {
        Jazz2FormatDataBlock headerBlock(fh.read(262), false, 262);

        // Skip the copyright info at the beginning of the file
        headerBlock.discardBytes(180);

        // Read the next four bytes; should spell out "TILE"
        qint32 id = headerBlock.readUInt();
        if (id != 0x454C4954) {
            std::cerr << "ERROR: This doesn't look like a J2T file! (Invalid header)\n";
            throw 0;
        }
        quint32 hash = headerBlock.readUInt();
        if (hash != 0xAFBEADDE) {
            std::cerr << "ERROR: This doesn't look like a J2T file! (Invalid magic number)\n";
        }
            
        // Read and display the level title
        QString tilesetname = headerBlock.readRawBytes(32);
        std::cout << "Tileset name: " << tilesetname.toStdString() << "\n";
            
        // Check version and report the JCS version
        qint16 version = headerBlock.readUShort();
        std::cout << "JCS version: " << (version % 256) << ".0" << (version / 256) << " (" << (version <= 257 ? "base game" : "TSF/CC") << ")\n";
            
        // Choose the tiles mode according to the said version
        const uint maxTiles = (version <= 257) ? 1024 : 4096;
            
        quint32 filesize_listed = headerBlock.readUInt();

        if (filesize != filesize_listed) {
            std::cout << "WARNING: File size doesn't match the one saved inside the file!\n"
                        << "(expected " << filesize_listed << "bytes, length was " << filesize << ")\n";
        }
            
        // Get the CRC; would check here if it matches if we knew what variant it is AND what it applies to
        // Test file across all CRC32 variants + Adler had no matches to the value obtained from the file
        // so either the variant is something else or the CRC is not applied to the whole file but on a part
        quint32 crc_listed = headerBlock.readUInt();
        
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

        // Parsing the info block
        std::cout << "\nParsing the information block...\n";
        QVector<sf::Color> palette;
        for (unsigned i = 0; i < 256; ++i) {
                
            sf::Uint8 red   = infoBlock.readChar();
            sf::Uint8 green = infoBlock.readChar();
            sf::Uint8 blue  = infoBlock.readChar();
            sf::Uint8 alpha = infoBlock.readChar();
            sf::Color color(red, green, blue, 255 - alpha);
            std::cout << "Palette color N:o " << (i + 1) 
                      << ": RGB #" << QString::number(red,   16).rightJustified(2, '0').toStdString()
                                   << QString::number(green, 16).rightJustified(2, '0').toStdString()
                                   << QString::number(blue,  16).rightJustified(2, '0').toStdString()
                                   << " alpha: " << QString::number(255 - alpha, 10).toStdString() << "\n";
            palette << color;
        }
            
        qint32 tileCount = infoBlock.readUInt();
        std::cout << "Tile count: " << tileCount << "\n";

        QVector<J2Tile*> tiles;
        for (int i = 0; i < maxTiles; ++i) {
            bool opaque = (infoBlock.readBool());
            J2Tile* tile = new J2Tile();
            tile->opaque = opaque;
            tiles << tile;
        }

        // block of unknown values, skip
        infoBlock.discardBytes(maxTiles);
            
        std::cout << "Parsing the tile-image assignments...\n";
        for (int i = 0; i < maxTiles; ++i) {
            tiles.at(i)->imageIdx = infoBlock.readUInt() / 1024;
        }

        // block of unknown values, skip
        infoBlock.discardBytes(4 * maxTiles);
            
        std::cout << "Parsing the tile-transparency mask assignments...\n";
        for (int i = 0; i < maxTiles; ++i) {
            tiles.at(i)->alphaIdx = infoBlock.readUInt();
        }
            
        // block of unknown values, skip
        infoBlock.discardBytes(4 * maxTiles);
            
        std::cout << "Parsing the tile-mask assignments...\n";
        for (int i = 0; i < maxTiles; ++i) {
            tiles.at(i)->maskIdx = infoBlock.readUInt() / 128;
        }

        // we don't care about the flipped masks, those are generated on runtime
        infoBlock.discardBytes(4 * maxTiles);
            
        // Parsing the image block
        std::cout << "\nParsing the image block...\n";
        QVector<sf::Image*> tile_imgs;
        for (int i = 0; i < (imageBlockUnpackedSize / 1024); ++i) {
            sf::Image* img = new sf::Image();
            img->create(32, 32, sf::Color(0, 0, 0, 255));
            for (int j = 0; j < 1024; ++j) {
                unsigned char idx = imageBlock.readChar();
                img->setPixel(j % 32, j / 32, palette.at(idx));
            }
            tile_imgs << img;
        }
            
        // Parsing the transparency block
        std::cout << "Parsing the transparency block...\n";
        for (int i = 0; i < tiles.size(); ++i) {
            int offset = tiles.at(i)->alphaIdx;
            tiles.at(i)->alphaMask.resize(1024);
            QByteArray alphaMaskData = alphaBlock.readRawBytes(128, offset);
            for (int j = 0; j < 128; ++j) {
                assert(offset < alphaBlockUnpackedSize);
                unsigned char byte = static_cast<unsigned char>(alphaMaskData.at(j));
                unsigned char bitmask = 0x01;
                for (int k = 0; k < 8; ++k) {
                    tiles.at(i)->alphaMask.setBit(j * 8 + k, ((bitmask & byte) == 0));
                    bitmask <<= 1;
                }
            }
        }
            
        // Parsing the mask block
        std::cout << "Parsing the mask block...\n";
        QVector<sf::Image*> maskTileList;
        for (int i = 0; i < (maskBlockUnpackedSize / 128); ++i) {
            sf::Image* img = new sf::Image();
            img->create(32,32,sf::Color::Black);
            for (int j = 0; j < 128; ++j) {
                unsigned char byte = maskBlock.readChar();
                unsigned char bitmask = 0x01;
                for (int k = 0; k < 8; ++k) {
                    if ((bitmask & byte) == 0) {
                        img->setPixel((8 * j + k) % 32, (8 * j + k) / 32, sf::Color(0,0,0,0));
                    }
                    bitmask *= 2;
                }
            }
            maskTileList << img;
        }
            
        QDir outdir(QDir::current());
        if (!outdir.mkdir(unique_id)) {
            int idx = 0;
            while (!outdir.mkdir(unique_id + "_" + QString::number(idx))) {
                idx++;
            }
            outdir.cd(unique_id + "_" + QString::number(idx));
        } else {
            outdir.cd(unique_id);
        }
            
        std::string tilefile = outdir.absoluteFilePath("tiles.png").toStdString();
        std::string maskfile = outdir.absoluteFilePath("mask.png").toStdString();
            
        std::cout << "\nRendering the image files...\n";
        sf::RenderTexture tilesTexture;
        sf::RenderTexture masksTexture;
        tilesTexture.create(320,tileCount / 10 * 32);
        tilesTexture.clear(sf::Color(0, 0, 0, 0));
        masksTexture.create(320,tileCount / 10 * 32);
        masksTexture.clear(sf::Color(0, 0, 0, 0));
        sf::Texture tex;
        tex.create(32, 32);
        sf::Sprite spr;
        spr.setTexture(tex,true);
        for (int i = 0; i < tileCount; ++i) {
            spr.setPosition((i % 10) * 32, (i / 10) * 32);
            sf::Image newtile(*(tile_imgs.at(tiles.at(i)->imageIdx)));

            // Apply transparency
            for (int j = 0; j < 1024; ++j) {
                if (tiles.at(i)->alphaMask.at(j)) {
                    newtile.setPixel(j % 32, j / 32, sf::Color(0, 0, 0, 0));
                }
            }

            tex.loadFromImage(newtile);
            tilesTexture.draw(spr);
                
            tex.update(*(maskTileList.at(tiles.at(i)->maskIdx)));
            masksTexture.draw(spr);
        }
        tilesTexture.display();
        masksTexture.display();
        sf::Image final_img = tilesTexture.getTexture().copyToImage();
        final_img.saveToFile(tilefile);
            
        final_img = masksTexture.getTexture().copyToImage();
        final_img.createMaskFromColor(sf::Color::White, 0);
        final_img.saveToFile(maskfile);

    } catch (Jazz2FormatParseException e) {
        std::cout << "ERROR: " << e.friendlyText().toStdString() << "\n";
        getchar();
        return EXIT_FAILURE;
    } catch (...) {
        std::cout << "Aborting conversion...\n";
        if (fh.isOpen()) {
            fh.close();
        }
        return EXIT_FAILURE;
    }
}
