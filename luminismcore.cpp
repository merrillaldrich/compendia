#include "luminismcore.h"

LuminismCore::LuminismCore(QObject *parent)
    : QObject{parent}
{}

void LuminismCore::buttonClick(){
    qDebug() << "Click!";
}
