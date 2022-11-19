#ifndef ANIMATIONMODEL_H
#define ANIMATIONMODEL_H

#include <QAbstractListModel>
#include <QObject>

#include <vsg/nodes/Node.h>
#include "animated-model.h"

class AnimationModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit AnimationModel(vsg::ref_ptr<AnimatedModel> model, QObject *parent = nullptr);

    virtual ~AnimationModel();

    void setModel(vsg::ref_ptr<AnimatedModel> model);
    void addAnimation(QString key, vsg::ref_ptr<Animation> animation);
    void addBase(vsg::ref_ptr<vsg::Node> base);

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

private:
    vsg::ref_ptr<AnimatedModel> _model;
};

#endif // ANIMATIONMODEL_H
