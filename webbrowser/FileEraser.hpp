#include <QFile>

class FileEraser: public QObject
{
    Q_OBJECT

    public:
        Q_INVOKABLE bool eraseFile(const QString& path);
};
