
#include "pcapfileformat.h"
#include "pdmlfileformat.h"
#include "protocol.pb.h"
#include "protocolmanager.h"

#include <QCoreApplication>
#include <QString>

extern ProtocolManager *OstProtocolManager;

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QString error;

    if (argc != 3)
    {
        printf("%s <infile> <outfile>\n", argv[0]);
        exit(255);
    }

    OstProtocolManager = new ProtocolManager();

    OstProto::StreamConfigList streams;
    QString inFile(argv[1]);
    QString outFile(argv[2]);

    if (!pcapFileFormat.openStreams(inFile, streams, error))
    {
        fprintf(stdout, "failed reading streams from %s:%s\n", 
                inFile.toAscii().constData(), error.toAscii().constData());
        exit(1);
    }

    if (!pcapFileFormat.saveStreams(streams, outFile, error))
    {
        fprintf(stdout, "failed writing streams to %s:%s\n", 
                outFile.toAscii().constData(), error.toAscii().constData());
        exit(1);
    }

    return 0;
}
