#ifndef _PBQTIO_H
#define _PBQTIO_H

#include <google/protobuf/io/zero_copy_stream_impl.h>

class PbQtInputStream : public google::protobuf::io::CopyingInputStream
{
public:
    PbQtInputStream(QIODevice *dev) 
        : dev_(dev) {};
    int Read(void *buffer, int size) {
    _top:
        if (dev_->bytesAvailable())
            return dev_->read(static_cast<char*>(buffer), size);
        else
            if (dev_->waitForReadyRead(-1))
                goto _top;
            else
                return -1; //return dev_->atEnd() ? 0 : -1;
    }

private:
    QIODevice *dev_;
};

class PbQtOutputStream : public google::protobuf::io::CopyingOutputStream
{
public:
    PbQtOutputStream(QIODevice *dev) 
        : dev_(dev) {};
    bool Write(const void *buffer, int size) {
        if (dev_->write(static_cast<const char*>(buffer), size) == size)
            return true;
        else
            return false;
    }

private:
    QIODevice *dev_;
};

#endif
