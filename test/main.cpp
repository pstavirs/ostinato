
#include "ostprotolib.h"
#include "pcapfileformat.h"
#include "protocol.pb.h"
#include "protocolmanager.h"
#include "settings.h"

#include <QCoreApplication>
#include <QFile>
#include <QSettings>
#include <QString>

extern ProtocolManager *OstProtocolManager;

QSettings *appSettings;

#if defined(Q_OS_WIN32)
QString kGzipPathDefaultValue;
QString kDiffPathDefaultValue;
QString kAwkPathDefaultValue;
#endif

int usage(int argc, char* argv[])
{
    printf("usage:\n");
    printf("%s <command>\n", argv[0]);
    printf("command -\n");
    printf("  importpcap\n");

    return 255;
}

int testImportPcap(int argc, char* argv[])
{
    bool isOk;
    QString error;

    if (argc != 3)
    {
        printf("usage:\n");
        printf("%s importpcap <pcapfile>\n", argv[0]);
        return 255;
    }

    OstProto::StreamConfigList streams;
    QString inFile(argv[2]);

    isOk = pcapFileFormat.openStreams(inFile, streams, error);
    if (!error.isEmpty())
    {
        printf("%s: %s\n", 
                inFile.toAscii().constData(), error.toAscii().constData());
    }

    if (!isOk)
        return 1;

    return 0;
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    int exitCode = 0;

    // app init starts ...
#if defined(Q_OS_WIN32)
    kGzipPathDefaultValue = app.applicationDirPath() + "/gzip.exe";
    kDiffPathDefaultValue = app.applicationDirPath() + "/diff.exe";
    kAwkPathDefaultValue  = app.applicationDirPath() + "/gawk.exe";
#endif

    app.setApplicationName("Ostinato");
    app.setOrganizationName("Ostinato");

    OstProtocolManager = new ProtocolManager();

    /* (Portable Mode) If we have a .ini file in the same directory as the 
       executable, we use that instead of the platform specific location
       and format for the settings */
    QString portableIni = QCoreApplication::applicationDirPath() 
            + "/ostinato.ini";
    if (QFile::exists(portableIni))
        appSettings = new QSettings(portableIni, QSettings::IniFormat);
    else
        appSettings = new QSettings();

    OstProtoLib::setExternalApplicationPaths(
        appSettings->value(kTsharkPathKey, kTsharkPathDefaultValue).toString(),
        appSettings->value(kGzipPathKey, kGzipPathDefaultValue).toString(),
        appSettings->value(kDiffPathKey, kDiffPathDefaultValue).toString(),
        appSettings->value(kAwkPathKey, kAwkPathDefaultValue).toString());

    // ... app init finished

    //
    // identify and run specified test
    //
    if (argc < 2)
        exitCode = usage(argc, argv);
    else if (strcmp(argv[1],"importpcap") == 0)
        exitCode = testImportPcap(argc, argv);
    else
        exitCode = usage(argc, argv);

    delete appSettings;
    return exitCode;
}

