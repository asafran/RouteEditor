#ifndef ANIMATIONMODEL_H
#define ANIMATIONMODEL_H

#include <QAbstractListModel>
#include <QObject>

#include <vsg/nodes/Node.h>
#include "sceneobjects.h"

class AnimationModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit AnimationModel(vsg::ref_ptr<route::AnimatedObject> model, QObject *parent = nullptr);

    virtual ~AnimationModel();

    void setModel(vsg::ref_ptr<route::AnimatedObject> model);
    QModelIndex addAnimation(QString key, vsg::ref_ptr<Animation> animation);
    void addBase(vsg::ref_ptr<vsg::Node> base);
    void remove(const QModelIndex &index);

    Animation *animationIndex(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

private:
    vsg::ref_ptr<route::AnimatedObject> _model;
};

#endif // ANIMATIONMODEL_H
