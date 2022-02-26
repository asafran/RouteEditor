#include "lightanimation.h"

LightAnimation::LightAnimation(vsg::ref_ptr<vsg::Light> l, QObject *parent)
    : QVariantAnimation{parent}
    , _light(l)
{

}


void LightAnimation::updateCurrentValue(const QVariant &value)
{
    _light->intensity = value.toFloat();
}
