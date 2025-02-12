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

#include <algorithm>

#include <qvariantanimation.h>
#include <qvariantanimation_p.h>

#include <QtCore/qrect.h>
#include <QtCore/qline.h>
#include <QtCore/qmutex.h>
#include <qmutexpool_p.h>

#ifndef QT_NO_ANIMATION

QT_BEGIN_NAMESPACE

static bool animationValueLessThan(const QVariantAnimation::KeyValue &p1, const QVariantAnimation::KeyValue &p2)
{
   return p1.first < p2.first;
}

static QVariant defaultInterpolator(const void *, const void *, qreal)
{
   return QVariant();
}

template<>
inline QRect _q_interpolate(const QRect &f, const QRect &t, qreal progress)
{
   QRect ret;
   ret.setCoords(_q_interpolate(f.left(), t.left(), progress),
                 _q_interpolate(f.top(), t.top(), progress),
                 _q_interpolate(f.right(), t.right(), progress),
                 _q_interpolate(f.bottom(), t.bottom(), progress));
   return ret;
}

template<>
inline QRectF _q_interpolate(const QRectF &f, const QRectF &t, qreal progress)
{
   qreal x1, y1, w1, h1;
   f.getRect(&x1, &y1, &w1, &h1);
   qreal x2, y2, w2, h2;
   t.getRect(&x2, &y2, &w2, &h2);
   return QRectF(_q_interpolate(x1, x2, progress), _q_interpolate(y1, y2, progress),
                 _q_interpolate(w1, w2, progress), _q_interpolate(h1, h2, progress));
}

template<>
inline QLine _q_interpolate(const QLine &f, const QLine &t, qreal progress)
{
   return QLine( _q_interpolate(f.p1(), t.p1(), progress), _q_interpolate(f.p2(), t.p2(), progress));
}

template<>
inline QLineF _q_interpolate(const QLineF &f, const QLineF &t, qreal progress)
{
   return QLineF( _q_interpolate(f.p1(), t.p1(), progress), _q_interpolate(f.p2(), t.p2(), progress));
}

QVariantAnimationPrivate::QVariantAnimationPrivate() : duration(250), interpolator(&defaultInterpolator)
{ }

void QVariantAnimationPrivate::convertValues(int t)
{
   //this ensures that all the keyValues are of type t
   for (int i = 0; i < keyValues.count(); ++i) {
      QVariantAnimation::KeyValue &pair = keyValues[i];
      pair.second.convert(static_cast<QVariant::Type>(t));
   }
   //we also need update to the current interval if needed
   currentInterval.start.second.convert(static_cast<QVariant::Type>(t));
   currentInterval.end.second.convert(static_cast<QVariant::Type>(t));

   //... and the interpolator
   updateInterpolator();
}

void QVariantAnimationPrivate::updateInterpolator()
{
   int type = currentInterval.start.second.userType();
   if (type == currentInterval.end.second.userType()) {
      interpolator = getInterpolator(type);
   } else {
      interpolator = 0;
   }

   //we make sure that the interpolator is always set to something
   if (!interpolator) {
      interpolator = &defaultInterpolator;
   }
}

/*!
    \internal
    The goal of this function is to update the currentInterval member. As a consequence, we also
    need to update the currentValue.
    Set \a force to true to always recalculate the interval.
*/
void QVariantAnimationPrivate::recalculateCurrentInterval(bool force/*=false*/)
{
   // can't interpolate if we don't have at least 2 values
   if ((keyValues.count() + (defaultStartEndValue.isValid() ? 1 : 0)) < 2) {
      return;
   }

   const qreal endProgress = (direction == QAbstractAnimation::Forward) ? qreal(1) : qreal(0);
   const qreal progress = easing.valueForProgress(((duration == 0) ? endProgress : qreal(currentTime) / qreal(duration)));

   //0 and 1 are still the boundaries
   if (force || (currentInterval.start.first > 0 && progress < currentInterval.start.first)
         || (currentInterval.end.first < 1 && progress > currentInterval.end.first)) {

      //let's update currentInterval
      QVariantAnimation::KeyValues::const_iterator it = std::lower_bound(keyValues.constBegin(),
            keyValues.constEnd(),
            qMakePair(progress, QVariant()),
            animationValueLessThan);

      if (it == keyValues.constBegin()) {
         //the item pointed to by it is the start element in the range
         if (it->first == 0 && keyValues.count() > 1) {
            currentInterval.start = *it;
            currentInterval.end = *(it + 1);
         } else {
            currentInterval.start = qMakePair(qreal(0), defaultStartEndValue);
            currentInterval.end = *it;
         }
      } else if (it == keyValues.constEnd()) {
         --it; //position the iterator on the last item
         if (it->first == 1 && keyValues.count() > 1) {
            //we have an end value (item with progress = 1)
            currentInterval.start = *(it - 1);
            currentInterval.end = *it;
         } else {
            //we use the default end value here
            currentInterval.start = *it;
            currentInterval.end = qMakePair(qreal(1), defaultStartEndValue);
         }
      } else {
         currentInterval.start = *(it - 1);
         currentInterval.end = *it;
      }

      // update all the values of the currentInterval
      updateInterpolator();
   }
   setCurrentValueForProgress(progress);
}

void QVariantAnimationPrivate::setCurrentValueForProgress(const qreal progress)
{
   Q_Q(QVariantAnimation);

   const qreal startProgress = currentInterval.start.first;
   const qreal endProgress = currentInterval.end.first;
   const qreal localProgress = (progress - startProgress) / (endProgress - startProgress);

   QVariant ret = q->interpolated(currentInterval.start.second, currentInterval.end.second, localProgress);
   qSwap(currentValue, ret);
   q->updateCurrentValue(currentValue);

   emit q->valueChanged(currentValue);
}

QVariant QVariantAnimationPrivate::valueAt(qreal step) const
{
   QVariantAnimation::KeyValues::const_iterator result =
      std::lower_bound(keyValues.constBegin(), keyValues.constEnd(), qMakePair(step, QVariant()), animationValueLessThan);

   if (result != keyValues.constEnd() && ! animationValueLessThan(qMakePair(step, QVariant()), *result)) {
      return result->second;
   }

   return QVariant();
}

void QVariantAnimationPrivate::setValueAt(qreal step, const QVariant &value)
{
   if (step < qreal(0.0) || step > qreal(1.0)) {
      qWarning("QVariantAnimation::setValueAt: invalid step = %f", step);
      return;
   }

   QVariantAnimation::KeyValue pair(step, value);

   QVariantAnimation::KeyValues::iterator result = std::lower_bound(keyValues.begin(), keyValues.end(), pair,
         animationValueLessThan);
   if (result == keyValues.end() || result->first != step) {
      keyValues.insert(result, pair);
   } else {
      if (value.isValid()) {
         result->second = value;   // replaces the previous value
      } else {
         keyValues.erase(result);   // removes the previous value
      }
   }

   recalculateCurrentInterval(/*force=*/true);
}

void QVariantAnimationPrivate::setDefaultStartEndValue(const QVariant &value)
{
   defaultStartEndValue = value;
   recalculateCurrentInterval(/*force=*/true);
}

/*!
    Construct a QVariantAnimation object. \a parent is passed to QAbstractAnimation's
    constructor.
*/
QVariantAnimation::QVariantAnimation(QObject *parent) : QAbstractAnimation(*new QVariantAnimationPrivate, parent)
{
}

/*!
    \internal
*/
QVariantAnimation::QVariantAnimation(QVariantAnimationPrivate &dd, QObject *parent) : QAbstractAnimation(dd, parent)
{
}

/*!
    Destroys the animation.
*/
QVariantAnimation::~QVariantAnimation()
{
}

/*!
    \property QVariantAnimation::easingCurve
    \brief the easing curve of the animation

    This property defines the easing curve of the animation. By
    default, a linear easing curve is used, resulting in linear
    interpolation. Other curves are provided, for instance,
    QEasingCurve::InCirc, which provides a circular entry curve.
    Another example is QEasingCurve::InOutElastic, which provides an
    elastic effect on the values of the interpolated variant.

    QVariantAnimation will use the QEasingCurve::valueForProgress() to
    transform the "normalized progress" (currentTime / totalDuration)
    of the animation into the effective progress actually
    used by the animation. It is this effective progress that will be
    the progress when interpolated() is called. Also, the steps in the
    keyValues are referring to this effective progress.

    The easing curve is used with the interpolator, the interpolated()
    virtual function, the animation's duration, and iterationCount, to
    control how the current value changes as the animation progresses.
*/
QEasingCurve QVariantAnimation::easingCurve() const
{
   Q_D(const QVariantAnimation);
   return d->easing;
}

void QVariantAnimation::setEasingCurve(const QEasingCurve &easing)
{
   Q_D(QVariantAnimation);
   d->easing = easing;
   d->recalculateCurrentInterval();
}

typedef QVector<QVariantAnimation::Interpolator> QInterpolatorVector;
Q_GLOBAL_STATIC(QInterpolatorVector, registeredInterpolators)

/*!
    \fn void qRegisterAnimationInterpolator(QVariant (*func)(const T &from, const T &to, qreal progress))
    \relates QVariantAnimation
    \threadsafe

    Registers a custom interpolator \a func for the template type \c{T}.
    The interpolator has to be registered before the animation is constructed.
    To unregister (and use the default interpolator) set \a func to 0.
 */

/*!
    \internal
    \typedef QVariantAnimation::Interpolator

    This is a typedef for a pointer to a function with the following
    signature:
    \code
    QVariant myInterpolator(const QVariant &from, const QVariant &to, qreal progress);
    \endcode

*/

/*! \internal
 * Registers a custom interpolator \a func for the specific \a interpolationType.
 * The interpolator has to be registered before the animation is constructed.
 * To unregister (and use the default interpolator) set \a func to 0.
 */
void QVariantAnimation::registerInterpolator(QVariantAnimation::Interpolator func, int interpolationType)
{
   // will override any existing interpolators
   QInterpolatorVector *interpolators = registeredInterpolators();
   // When built on solaris with GCC, the destructors can be called
   // in such an order that we get here with interpolators == NULL,
   // to continue causes the app to crash on exit with a SEGV
   if (interpolators) {

      QMutexLocker locker(QMutexPool::globalInstanceGet(interpolators));

      if (int(interpolationType) >= interpolators->count()) {
         interpolators->resize(int(interpolationType) + 1);
      }
      interpolators->replace(interpolationType, func);
   }
}


template<typename T> static inline QVariantAnimation::Interpolator castToInterpolator(QVariant (*func)(const T &from,
      const T &to, qreal progress))
{
   return reinterpret_cast<QVariantAnimation::Interpolator>(func);
}

QVariantAnimation::Interpolator QVariantAnimationPrivate::getInterpolator(int interpolationType)
{
   QInterpolatorVector *interpolators = registeredInterpolators();

   QMutexLocker locker(QMutexPool::globalInstanceGet(interpolators));

   QVariantAnimation::Interpolator ret = 0;
   if (interpolationType < interpolators->count()) {
      ret = interpolators->at(interpolationType);
      if (ret) {
         return ret;
      }
   }

   switch (interpolationType) {
      case QMetaType::Int:
         return castToInterpolator(_q_interpolateVariant<int>);
      case QMetaType::UInt:
         return castToInterpolator(_q_interpolateVariant<uint>);
      case QMetaType::Double:
         return castToInterpolator(_q_interpolateVariant<double>);
      case QMetaType::Float:
         return castToInterpolator(_q_interpolateVariant<float>);
      case QMetaType::QLine:
         return castToInterpolator(_q_interpolateVariant<QLine>);
      case QMetaType::QLineF:
         return castToInterpolator(_q_interpolateVariant<QLineF>);
      case QMetaType::QPoint:
         return castToInterpolator(_q_interpolateVariant<QPoint>);
      case QMetaType::QPointF:
         return castToInterpolator(_q_interpolateVariant<QPointF>);
      case QMetaType::QSize:
         return castToInterpolator(_q_interpolateVariant<QSize>);
      case QMetaType::QSizeF:
         return castToInterpolator(_q_interpolateVariant<QSizeF>);
      case QMetaType::QRect:
         return castToInterpolator(_q_interpolateVariant<QRect>);
      case QMetaType::QRectF:
         return castToInterpolator(_q_interpolateVariant<QRectF>);
      default:
         return 0; //this type is not handled
   }
}

/*!
    \property QVariantAnimation::duration
    \brief the duration of the animation

    This property describes the duration in milliseconds of the
    animation. The default duration is 250 milliseconds.

    \sa QAbstractAnimation::duration()
 */
int QVariantAnimation::duration() const
{
   Q_D(const QVariantAnimation);
   return d->duration;
}

void QVariantAnimation::setDuration(int msecs)
{
   Q_D(QVariantAnimation);
   if (msecs < 0) {
      qWarning("QVariantAnimation::setDuration: cannot set a negative duration");
      return;
   }
   if (d->duration == msecs) {
      return;
   }
   d->duration = msecs;
   d->recalculateCurrentInterval();
}

/*!
    \property QVariantAnimation::startValue
    \brief the optional start value of the animation

    This property describes the optional start value of the animation. If
    omitted, or if a null QVariant is assigned as the start value, the
    animation will use the current position of the end when the animation
    is started.

    \sa endValue
*/
QVariant QVariantAnimation::startValue() const
{
   return keyValueAt(0);
}

void QVariantAnimation::setStartValue(const QVariant &value)
{
   setKeyValueAt(0, value);
}

/*!
    \property QVariantAnimation::endValue
    \brief the end value of the animation

    This property describes the end value of the animation.

    \sa startValue
 */
QVariant QVariantAnimation::endValue() const
{
   return keyValueAt(1);
}

void QVariantAnimation::setEndValue(const QVariant &value)
{
   setKeyValueAt(1, value);
}


/*!
    Returns the key frame value for the given \a step. The given \a step
    must be in the range 0 to 1. If there is no KeyValue for \a step,
    it returns an invalid QVariant.

    \sa keyValues(), setKeyValueAt()
*/
QVariant QVariantAnimation::keyValueAt(qreal step) const
{
   return d_func()->valueAt(step);
}

/*!
    \typedef QVariantAnimation::KeyValue

    This is a typedef for QPair<qreal, QVariant>.
*/
/*!
    \typedef QVariantAnimation::KeyValues

    This is a typedef for QVector<KeyValue>
*/

/*!
    Creates a key frame at the given \a step with the given \a value.
    The given \a step must be in the range 0 to 1.

    \sa setKeyValues(), keyValueAt()
*/
void QVariantAnimation::setKeyValueAt(qreal step, const QVariant &value)
{
   d_func()->setValueAt(step, value);
}

/*!
    Returns the key frames of this animation.

    \sa keyValueAt(), setKeyValues()
*/
QVariantAnimation::KeyValues QVariantAnimation::keyValues() const
{
   return d_func()->keyValues;
}

/*!
    Replaces the current set of key frames with the given \a keyValues.
    the step of the key frames must be in the range 0 to 1.

    \sa keyValues(), keyValueAt()
*/
void QVariantAnimation::setKeyValues(const KeyValues &keyValues)
{
   Q_D(QVariantAnimation);
   d->keyValues = keyValues;
   std::sort(d->keyValues.begin(), d->keyValues.end(), animationValueLessThan);
   d->recalculateCurrentInterval(/*force=*/true);
}

QVariant QVariantAnimation::currentValue() const
{
   Q_D(const QVariantAnimation);
   if (!d->currentValue.isValid()) {
      const_cast<QVariantAnimationPrivate *>(d)->recalculateCurrentInterval();
   }
   return d->currentValue;
}

/*!
    \reimp
 */
bool QVariantAnimation::event(QEvent *event)
{
   return QAbstractAnimation::event(event);
}

/*!
    \reimp
*/
void QVariantAnimation::updateState(QAbstractAnimation::State newState,
                                    QAbstractAnimation::State oldState)
{
   Q_UNUSED(oldState);
   Q_UNUSED(newState);
}

/*!

    This virtual function returns the linear interpolation between
    variants \a from and \a to, at \a progress, usually a value
    between 0 and 1. You can reimplement this function in a subclass
    of QVariantAnimation to provide your own interpolation algorithm.

    Note that in order for the interpolation to work with a
    QEasingCurve that return a value smaller than 0 or larger than 1
    (such as QEasingCurve::InBack) you should make sure that it can
    extrapolate. If the semantic of the datatype does not allow
    extrapolation this function should handle that gracefully.

    You should call the QVariantAnimation implementation of this
    function if you want your class to handle the types already
    supported by Qt (see class QVariantAnimation description for a
    list of supported types).

    \sa QEasingCurve
 */
QVariant QVariantAnimation::interpolated(const QVariant &from, const QVariant &to, qreal progress) const
{
   return d_func()->interpolator(from.constData(), to.constData(), progress);
}

/*!
    \reimp
 */
void QVariantAnimation::updateCurrentTime(int)
{
   d_func()->recalculateCurrentInterval();
}

QT_END_NAMESPACE

#endif //QT_NO_ANIMATION
