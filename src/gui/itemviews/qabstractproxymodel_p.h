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

#ifndef QABSTRACTPROXYMODEL_P_H
#define QABSTRACTPROXYMODEL_P_H

#include <qabstractitemmodel_p.h>

#ifndef QT_NO_PROXYMODEL

QT_BEGIN_NAMESPACE

class QAbstractProxyModelPrivate : public QAbstractItemModelPrivate
{
   Q_DECLARE_PUBLIC(QAbstractProxyModel)

 public:
   QAbstractProxyModelPrivate() : QAbstractItemModelPrivate(), model(0) {}
   QAbstractItemModel *model;
   virtual void _q_sourceModelDestroyed();
};

QT_END_NAMESPACE

#endif // QT_NO_PROXYMODEL

#endif // QABSTRACTPROXYMODEL_P_H
