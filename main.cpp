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

    QFile input(xmlFilename);
    if (!input.open(QIODevice::ReadOnly|QIODevice::Text)) {
        qCritical() << "No file" << xmlFilename << "found.";
        return 1;
    }
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
                            bool wordOK = true;
                            int unmatchedChars = 0;
                            foreach (QChar character, charMatchList) {
                                if (!readWord.contains(character, Qt::CaseInsensitive)) {
                                    wordOK = false;
                                    break;
                                } else if (readWord.count(character) != charMatchList.count(character)) {
                                    if (!unmatchedChars) {
                                        unmatchedChars++;
                                    } else {
                                        wordOK = false;
                                        break;
                                    }
                                }
                            }
                            if (wordOK &&
                                ((readWord.length() == charMatchList.length()) ||
                                 ((readWord.length()-1) == charMatchList.length())))
                                qDebug() << readWord;
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
    return 0;
}
