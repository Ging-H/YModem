#include "filetransmit.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FileTransmit w;
    w.show();

    return a.exec();
}
