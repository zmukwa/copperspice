/***********************************************************************
*
* Copyright (c) 2012-2019 Barbara Geller
* Copyright (c) 2012-2019 Ansel Sermersheim
*
* Copyright (C) 2015 The Qt Company Ltd.
* Copyright (c) 2012-2016 Digia Plc and/or its subsidiary(-ies).
* Copyright (c) 2008-2012 Nokia Corporation and/or its subsidiary(-ies).
*
* This file is part of CopperSpice.
*
* CopperSpice is free software. You can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License
* version 2.1 as published by the Free Software Foundation.
*
* CopperSpice is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* https://www.gnu.org/licenses/
*
***********************************************************************/

#ifndef QWINDOWSURFACE_QVFB_H
#define QWINDOWSURFACE_QVFB_H

#include <qwindowsurface_p.h>
#include <QPlatformWindow>

QT_BEGIN_NAMESPACE

class QVFbIntegration;
class QVFbScreen;

class QVFbWindowSurface : public QWindowSurface
{
public:
    QVFbWindowSurface(QVFbScreen *screen, QWidget *window);
    ~QVFbWindowSurface();

    QPaintDevice *paintDevice();
    void flush(QWidget *widget, const QRegion &region, const QPoint &offset);
    void resize(const QSize &size);

private:
    QVFbScreen *mScreen;
};

class QVFbWindow : public QPlatformWindow
{
public:
    QVFbWindow(QVFbScreen *screen, QWidget *window);
    void setGeometry(const QRect &rect);

private:
    QVFbScreen *mScreen;
};


QT_END_NAMESPACE

#endif
