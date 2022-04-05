#include "lightanimation.h"

LightAnimation::LightAnimation(vsg::ref_ptr<vsg::Light> l, QObject *parent)
    : QVariantAnimation{parent}
    , light(l)
{

}


void LightAnimation::updateCurrentValue(const QVariant &value)
{
    light->intensity = value.toFloat();
}
