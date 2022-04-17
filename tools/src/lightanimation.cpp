#include "lightanimation.h"

LightAnimation::LightAnimation(vsg::ref_ptr<vsg::Light> l, float intensity, int duration, QObject *parent)
    : QVariantAnimation{parent}
    , light(l)
{
    setDuration(duration);
    setStartValue(0.0f);
    setEndValue(intensity);
}


void LightAnimation::updateCurrentValue(const QVariant &value)
{
    light->intensity = value.toFloat();
}
