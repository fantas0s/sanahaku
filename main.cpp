#include <QCoreApplication>
#include <QCommandLineParser>
#include <QXmlStreamReader>
#include <QFile>
#include <QDebug>

static const QString xmlFilename = "kotus-sanalista_v1.xml";

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("sanahaku");
    QCoreApplication::setApplicationVersion("0.9.453.276-alpha");

    QCommandLineParser parser;
    parser.setApplicationDescription("Sanahaku - suomen kielen sanojen hakija.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption matchCharsOption(QStringList() << "s" << "string",
            QCoreApplication::translate("main", "Etsi annettuja kirjaimia. Jos sama kirjain on annettu useampaan kertaan, sen pitää esiintyä annetun määrän verran."),
            QCoreApplication::translate("main", "kirjainjono"));
    parser.addOption(matchCharsOption);
    QCommandLineOption unusedOption(QStringList() << "u" << "unused",
            QCoreApplication::translate("main", "Kuinka monta kirjainta saa jäädä käyttämättä."),
            QCoreApplication::translate("main", "numero"));
    parser.addOption(unusedOption);
    QCommandLineOption extraOption(QStringList() << "e" << "extra",
            QCoreApplication::translate("main", "Kuinka monta ylimääräistä kirjainta sallitaan."),
            QCoreApplication::translate("main", "numero"));
    parser.addOption(extraOption);

    // Process the actual command line arguments given by the user
    parser.process(app);

    QString charsToMatch = parser.value(matchCharsOption);
    QVector<QChar> charMatchList;
    while (charsToMatch.length()) {
        charMatchList.append(QChar(charsToMatch[0]));
        charsToMatch.remove(0,1);
    }
    if (!charMatchList.length()) {
        parser.showHelp();
        return 1;
    }

    QString limitString = parser.value(unusedOption);
    quint32 allowedUnusedChars = 0;
    if (limitString.length()) {
        allowedUnusedChars = limitString.toInt();
    }
    if (charMatchList.length() <= allowedUnusedChars) {
        parser.showHelp();
        return 1;
    }

    QString extraString = parser.value(extraOption);
    quint32 allowedExtraChars = 1;
    if (extraString.length()) {
        allowedExtraChars = extraString.toInt();
    }

    QFile input(xmlFilename);
    if (!input.open(QIODevice::ReadOnly|QIODevice::Text)) {
        qCritical() << "No file" << xmlFilename << "found.";
        return 1;
    }
    QTextStream myStdout( stdout );
    myStdout << "Allowing " << allowedUnusedChars << " unused characters.\n";
    myStdout << "Allowing " << allowedExtraChars << " extra characters.\n";
    QXmlStreamReader xmlReader(&input);
    /*
     * <kotus-sanalista>
<st><s>aakkonen</s><t><tn>38</tn></t></st>
     * */

    if (xmlReader.readNextStartElement()) {
        if (xmlReader.name() == "kotus-sanalista") {
            while(xmlReader.readNextStartElement()) {
                if(xmlReader.name() == "st") {
                    while(xmlReader.readNextStartElement()) {
                        if(xmlReader.name() == "s") {
                            const QString readWord = xmlReader.readElementText();
                            // The readWord must be built from the characters in charMatchList plus 1 or 0 extra characters.
                            bool wordOK = true;
                            int extraCharacters = 0;
                            for (int index = 0 ; index < readWord.length() ; ++index) {
                                if (!charMatchList.contains(readWord[index]))
                                {
                                    // The dictionary word has a character not in the given list
                                    if (extraCharacters < allowedExtraChars) {
                                        extraCharacters++;
                                    } else {
                                        wordOK = false;
                                        break;
                                    }
                                } else if (readWord.count(readWord[index]) > charMatchList.count(readWord[index])) {
                                    // The dictionary word has too many instances of the character in the given list
                                    if (extraCharacters <= allowedExtraChars) {
                                        extraCharacters += (readWord.count(readWord[index]) - charMatchList.count(readWord[index]));
                                    }
                                    if (extraCharacters > allowedExtraChars) {
                                        wordOK = false;
                                        break;
                                    }
                                }
                                // Otherwise OK.
                            }
                            if (wordOK &&
                                ((charMatchList.length() + extraCharacters) > (readWord.length()+allowedUnusedChars))) {
                                /* All of the characters in the dictionary word were in the list of given characters
                                 * (or "not found" characters is in allowed limits) but we have too many unused characters.
                                 */
                                wordOK = false;
                            }
                            if (wordOK) {
                                myStdout << readWord << "\n";
                            }
                        } else {
                            xmlReader.skipCurrentElement();
                        }
                    }
                } else {
                    xmlReader.skipCurrentElement();
                }
            }
        } else {
            xmlReader.raiseError(QString("Incorrect file"));
        }
    }
    myStdout.flush();
    return 0;
}
