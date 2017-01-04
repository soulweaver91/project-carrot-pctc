// J2T to native Project Carrot tileset file converter
// (c) 2013-2017 Soulweaver

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
#include "Jazz2Tileset.h"
#include "Version.h"

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

    try {
        Jazz2Tileset* tileset = Jazz2Tileset::fromFile(filename, false);
        tileset->saveAsProjectCarrotTileset(outdir, unique_id);
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
