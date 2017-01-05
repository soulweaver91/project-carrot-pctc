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
    if (argc < 2 || argc > 4) {
        std::cout << "PCTC version " << CONVERTERVERSION << "\n"
                  << "Usage: pctc name inputfile\n\n"
                  << "Converts a Jazz Jackrabbit 2 tileset file (.j2t) to the native, multiple file\n"
                  << "based format recognized by Project Carrot.\n\n"
                  << "Parameters:\n"
                  << "  name         Optional. The unique identifying name of this tileset. Use only\n"
                  << "               Latin letters, numbers and underscores. For the purposes of\n"
                  << "               being cross platform compliant, identifiers are case\n"
                  << "               insensitive.\n"
                  << "               If omitted, the name of the input file is used.\n"
                  << "  inputfile    Required. Complete or relative path to a J2T format file to be\n"
                  << "               converted.\n";
        return EXIT_FAILURE;
    }

    QString uniqueID;
    QString filename;
    if (argc == 3) {
        uniqueID = argv[1];
        filename = argv[2];
    } else {
        filename = argv[1];
        uniqueID = QFileInfo(filename).baseName();
    }

    QDir outdir(QDir::current());
    if (!outdir.mkdir(uniqueID)) {
        int idx = 0;
        while (!outdir.mkdir(uniqueID + "_" + QString::number(idx))) {
            idx++;
        }
        outdir.cd(uniqueID + "_" + QString::number(idx));
    } else {
        outdir.cd(uniqueID);
    }

    try {
        std::cout << "Converting \"" << filename.toStdString() << "\" to Project Carrot tileset \"" << uniqueID.toStdString() << "\"...\n";
        Jazz2Tileset* tileset = Jazz2Tileset::fromFile(filename, false);
        tileset->saveAsProjectCarrotTileset(outdir, uniqueID);
    } catch (Jazz2FormatParseException e) {
        std::cout << "ERROR: " << e.friendlyText().toStdString() << "\n";
        getchar();
        return EXIT_FAILURE;
    } catch (...) {
        std::cout << "Aborting conversion...\n";
        getchar();
        return EXIT_FAILURE;
    }
}
