#include <QApplication>
#include <QPushButton>
#include <QtWebKit/QWebView>
#include <QtWebKit/QWebFrame>
#include <QUrl>

#include "FileEraser.hpp"

int main(int argc, char **argv)
{
    QApplication application(argc, argv);

    QWebView view;
    view.load(QUrl(argc > 1 ? argv[1] : "https://www.google.com/"));
    view.show();

    FileEraser fileEraser;

    QWebFrame *frame = view.page()->mainFrame();
    frame->addToJavaScriptWindowObject("FileEraser", &fileEraser);

    return application.exec();
}
