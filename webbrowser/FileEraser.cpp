#include "FileEraser.hpp"

bool FileEraser::eraseFile(const QString& path)
{
    return QFile::remove(path);
}
