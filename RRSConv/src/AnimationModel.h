#ifndef ANIMATIONMODEL_H
#define ANIMATIONMODEL_H

#include <QAbstractListModel>
#include <QObject>

#include <vsg/nodes/Node.h>
#include "animation-model.h"

class AnimationModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit AnimationModel(QObject *parent = nullptr);

    virtual ~AnimationModel();

    void setModel(AnimationModel *model);

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

private:
    vsg::ref_ptr<AnimationModel> _model;
};

#endif // ANIMATIONMODEL_H
