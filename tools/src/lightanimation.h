#ifndef LIGHTANIMATION_H
#define LIGHTANIMATION_H

#include <QVariantAnimation>
#include <QObject>
#include <vsg/nodes/Light.h>

class LightAnimation : public QVariantAnimation
{
    Q_OBJECT
public:
    explicit LightAnimation(vsg::ref_ptr<vsg::Light> l, QObject *parent = nullptr);

    vsg::ref_ptr<vsg::Light> _light;

    // QVariantAnimation interface
protected:
    void updateCurrentValue(const QVariant &value);
};

#endif // LIGHTANIMATION_H
