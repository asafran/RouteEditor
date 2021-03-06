#ifndef LIGHTANIMATION_H
#define LIGHTANIMATION_H

#include <QVariantAnimation>
#include <QObject>
#include <vsg/nodes/Light.h>

class LightAnimation : public QVariantAnimation
{
    Q_OBJECT
public:
    explicit LightAnimation(vsg::ref_ptr<vsg::Light> l, float intensity, int duration, QObject *parent = nullptr);

    vsg::ref_ptr<vsg::Light> light;

    // QVariantAnimation interface
protected:
    void updateCurrentValue(const QVariant &value);
};

#endif // LIGHTANIMATION_H
