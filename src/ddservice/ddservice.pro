TARGET = DDService
CONFIG -= qt
CONFIG *= console
TEMPLATE = app
include(../common.pri)
LIBS *= -lAdvapi32
include(../ddutils/ddutils.pri)
SOURCES *= main.cpp
