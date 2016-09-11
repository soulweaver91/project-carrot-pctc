// J2T to native Project Carrot tileset file converter
// Written in 2013 by Soulweaver

#define CONVERTERVERSION "1.0.1"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <QDataStream>
#include <QByteArray>
#include <QBitArray>
#include <QString>
#include <QFile>
#include <QDir>
#include <iostream>
#include <cassert>

quint32 uintFromArray(const QByteArray& array) {
    if (array.size() != 4) {
        return 0;
    }
    QDataStream a(array);
    quint32 res = 0;
    quint8 byte;
    while (!a.atEnd()) {
        res *= 256;
        a >> byte;
        res += byte;
    }
    return res;
}

quint16 ushortFromArray(const QByteArray& array) {
    if (array.size() != 2) {
        return 0;
    }
    QDataStream a(array);
    quint32 res = 0;
    quint8 byte;
    while (!a.atEnd()) {
        res *= 256;
        a >> byte;
        res += byte;
    }
    return res;
}

QByteArray BEfromLE(QByteArray le) {
    QDataStream a(le);
    QByteArray be;
    a.setByteOrder(QDataStream::BigEndian);
    qint8 byte;
    while (!a.atEnd()) {
        a >> byte;
        be.prepend(byte);
    }
    return be;
}

struct J2Tile {
    bool opaque;
    quint32 image_idx;
    quint32 trans_idx;
    quint32 mask_idx;
    QBitArray trans_mask;
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
        // Skip the copyright info at the beginning of the file
        QByteArray r = fh.read(180);

        // Read the next four bytes; should spell out "TILE"
        qint32 id = uintFromArray(fh.read(4));
        if (id != 0x54494C45) {
            std::cerr << "ERROR: This doesn't look like a J2T file! (Invalid header)\n";
            throw 0;
        }
        quint32 hash = uintFromArray(fh.read(4));
        if (hash != 0xDEADBEAF) {
            std::cerr << "ERROR: This doesn't look like a J2T file! (Invalid magic number)\n";
        }
            
        // Read and display the level title
        QString tilesetname(fh.read(32));
        std::cout << "Tileset name: " << tilesetname.toLatin1().data() << "\n";
            
        // Check version and report the JCS version
        qint16 version(ushortFromArray(fh.read(2)));
        std::cout << "JCS version: " << (version % 256) << ".0" << (version / 256) << " (" << (version <= 257 ? "base game" : "TSF/CC") << ")\n";
            
        // Choose the tiles mode according to the said version
        const uint max_tiles = (version <= 257) ? 1024 : 4096;
            
        quint32 filesize_listed = uintFromArray(BEfromLE(fh.read(4)));

        if (filesize != filesize_listed) {
            std::cout << "WARNING: File size doesn't match the one saved inside the file!\n"
                        << "(expected " << filesize_listed << "bytes, length was " << filesize << ")\n";
        }
            
        // Get the CRC; would check here if it matches if we knew what variant it is AND what it applies to
        // Test file across all CRC32 variants + Adler had no matches to the value obtained from the file
        // so either the variant is something else or the CRC is not applied to the whole file but on a part
        quint32 crc_listed = uintFromArray(fh.read(4));
            
        // Read the lengths
        quint32 infoblock_packed_size        = uintFromArray(BEfromLE(fh.read(4)));
        QByteArray infoblock_unpacked_size   = BEfromLE(fh.read(4));
        quint32 imageblock_packed_size       = uintFromArray(BEfromLE(fh.read(4)));
        QByteArray imageblock_unpacked_size  = BEfromLE(fh.read(4));
        quint32 transblock_packed_size       = uintFromArray(BEfromLE(fh.read(4)));
        QByteArray transblock_unpacked_size  = BEfromLE(fh.read(4));
        quint32 maskblock_packed_size        = uintFromArray(BEfromLE(fh.read(4)));
        QByteArray maskblock_unpacked_size   = BEfromLE(fh.read(4));

        // Let's unpack all the blocks so we can let go of the original file
        QByteArray infoblock  = qUncompress( infoblock_unpacked_size + fh.read( infoblock_packed_size));
        QByteArray imageblock = qUncompress(imageblock_unpacked_size + fh.read(imageblock_packed_size));
        QByteArray transblock = qUncompress(transblock_unpacked_size + fh.read(transblock_packed_size));
        QByteArray maskblock  = qUncompress( maskblock_unpacked_size + fh.read( maskblock_packed_size));
            
        if (infoblock.size() == 0) {
            std::cerr << "ERROR: Unpacking the information block failed!\n";
            throw 0;
        }
        if (imageblock.size() == 0) {
            std::cerr << "ERROR: Unpacking the image block failed!\n";
            throw 0;
        }
        if (transblock.size() == 0) {
            std::cerr << "ERROR: Unpacking the transparency block failed!\n";
            throw 0;
        }
        if (maskblock.size() == 0) {
            std::cerr << "ERROR: Unpacking the mask block failed!\n";
            throw 0;
        }

        // we can now let go of the file
        fh.close();

        // Parsing the info block
        std::cout << "\nParsing the information block...\n";
        QList< sf::Color > palette;
        for (unsigned i = 0; i < 256; ++i) {
                
            sf::Uint8 red   = infoblock.at(0);
            sf::Uint8 green = infoblock.at(1);
            sf::Uint8 blue  = infoblock.at(2);
            sf::Uint8 alpha = infoblock.at(3);
            sf::Color a(red,green,blue,255-alpha);
            std::cout << "Palette color N:o " << (i+1) << ": RGB #" << QString::number(red,16).rightJustified(2,'0').toStdString()
                                                                    << QString::number(green,16).rightJustified(2,'0').toStdString()
                                                                    << QString::number(blue,16).rightJustified(2,'0').toStdString()
                                                                    << " alpha: " << QString::number(255-alpha,10).toStdString() << "\n";
            palette << a;
            infoblock.remove(0,4);
        }
            
        qint32 tiles_num = uintFromArray(BEfromLE(infoblock.left(4)));
        infoblock.remove(0,4);
        std::cout << "Tile count: " << tiles_num << "\n";

        QList< J2Tile* > tiles;
        for (int i = 0; i < max_tiles; ++i) {
            bool opaque = (infoblock.at(i) > 0);
            J2Tile* tile = new J2Tile();
            tile->opaque = opaque;
            tiles << tile;
        }
        infoblock.remove(0,max_tiles);

        // block of unknown values, skip
        infoblock.remove(0,max_tiles);
            
        std::cout << "Parsing the tile-image assignments...\n";
        for (int i = 0; i < max_tiles; ++i) {
            tiles.at(i)->image_idx = uintFromArray(BEfromLE(infoblock.mid(4 * i,4))) / 1024;
        }
        infoblock.remove(0,4 * max_tiles);

        // block of unknown values, skip
        infoblock.remove(0,4 * max_tiles);
            
        std::cout << "Parsing the tile-transparency mask assignments...\n";
        for (int i = 0; i < max_tiles; ++i) {
            tiles.at(i)->trans_idx = uintFromArray(BEfromLE(infoblock.mid(4 * i,4)));
        }
        infoblock.remove(0,4 * max_tiles);
            
        // block of unknown values, skip
        infoblock.remove(0,4 * max_tiles);
            
        std::cout << "Parsing the tile-mask assignments...\n";
        for (int i = 0; i < max_tiles; ++i) {
            tiles.at(i)->mask_idx = uintFromArray(BEfromLE(infoblock.mid(4 * i,4))) / 128;
        }
        infoblock.remove(0,4 * max_tiles);

        // we don't care about the flipped masks, those are generated on runtime
        infoblock.remove(0,4 * max_tiles);
        infoblock = "";
            
        // Parsing the image block
        std::cout << "\nParsing the image block...\n";
        QList< sf::Image* > tile_imgs;
        for (int i = 0; i < (uintFromArray(imageblock_unpacked_size) / 1024); ++i) {
            sf::Image* img = new sf::Image();
            img->create(32,32,sf::Color(0,0,0,255));
            for (int j = 0; j < 1024; ++j) {
                unsigned char idx = static_cast<unsigned char>(imageblock.at(j));
                img->setPixel(j % 32, j / 32, palette.at(idx));
            }
            tile_imgs << img;
            imageblock.remove(0,1024);
        }
            
        // Parsing the transparency block
        std::cout << "Parsing the transparency block...\n";
        for (int i = 0; i < tiles.size(); ++i) {
            int offset = tiles.at(i)->trans_idx;
            tiles.at(i)->trans_mask.resize(1024);
            for (int j = 0; j < 128; ++j) {
                assert(offset < transblock.size());
                unsigned char byte = transblock.at(offset + j);
                unsigned char bitmask = 0x01;
                for (int k = 0; k < 8; ++k) {
                    tiles.at(i)->trans_mask.setBit(j * 8 + k, ((bitmask & byte) == 0));
                    bitmask *= 2;
                }
            }
        }
            
        // Parsing the mask block
        std::cout << "Parsing the mask block...\n";
        QList< sf::Image* > tile_masks;
        for (int i = 0; i < (uintFromArray(maskblock_unpacked_size) / 128); ++i) {
            sf::Image* img = new sf::Image();
            img->create(32,32,sf::Color::Black);
            for (int j = 0; j < 128; ++j) {
                unsigned char byte = maskblock.at(j);
                unsigned char bitmask = 0x01;
                for (int k = 0; k < 8; ++k) {
                    if ((bitmask & byte) == 0) {
                        img->setPixel((8 * j + k) % 32, (8 * j + k) / 32, sf::Color(0,0,0,0));
                    }
                    bitmask *= 2;
                }
            }
            tile_masks << img;
            maskblock.remove(0,128);
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
        sf::RenderTexture target_tiles;
        sf::RenderTexture target_masks;
        target_tiles.create(320,tiles_num / 10 * 32);
        target_tiles.clear(sf::Color(0,0,0,0));
        target_masks.create(320,tiles_num / 10 * 32);
        target_masks.clear(sf::Color(0,0,0,0));
        sf::Texture tex;
        tex.create(32,32);
        sf::Sprite spr;
        spr.setTexture(tex,true);
        for (int i = 0; i < tiles_num; ++i) {
            spr.setPosition((i % 10) * 32, (i / 10) * 32);
            sf::Image newtile(*(tile_imgs.at(tiles.at(i)->image_idx)));

            // Apply transparency
            for (int j = 0; j < 1024; ++j) {
                if (tiles.at(i)->trans_mask.at(j)) {
                    newtile.setPixel(j % 32, j / 32, sf::Color(0,0,0,0));
                }
            }

            tex.loadFromImage(newtile);
            target_tiles.draw(spr);
                
            tex.update(*(tile_masks.at(tiles.at(i)->mask_idx)));
            target_masks.draw(spr);
        }
        target_tiles.display();
        target_masks.display();
        sf::Image final_img = target_tiles.getTexture().copyToImage();
        final_img.saveToFile(tilefile);
            
        final_img = target_masks.getTexture().copyToImage();
        final_img.createMaskFromColor(sf::Color::White,0);
        final_img.saveToFile(maskfile);

    } catch(...) {
        std::cout << "Aborting conversion...\n";
        if (fh.isOpen()) {
            fh.close();
        }
        return EXIT_FAILURE;
    }
}
