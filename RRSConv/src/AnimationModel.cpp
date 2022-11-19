#include "AnimationModel.h"

AnimationModel::AnimationModel(vsg::ref_ptr<AnimatedModel> model, QObject *parent)
    : QAbstractListModel{parent}
    , _model(model)
{

}

AnimationModel::~AnimationModel()
{

}

void AnimationModel::setModel(vsg::ref_ptr<AnimatedModel> model)
{
    beginResetModel();
    _model = model;
    endResetModel();
}

void AnimationModel::addAnimation(QString key, vsg::ref_ptr<Animation> animation)
{
    beginResetModel();
    _model->animations.insert({key.toStdString(), animation});
    endResetModel();
}

void AnimationModel::addBase(vsg::ref_ptr<vsg::Node> base)
{
    _model->addChild(base);
}

int AnimationModel::rowCount(const QModelIndex &parent) const
{
    return _model.valid() ? _model->animations.size() : 0;
}

QVariant AnimationModel::data(const QModelIndex &index, int role) const
{
    if (!_model || !index.isValid() || (role != Qt::DisplayRole && role != Qt::EditRole))
        return QVariant();
    Q_ASSERT(index.row() < _model->animations.size());
    auto it = std::next(_model->animations.cbegin(), index.row());
    return it->first.c_str();
}
