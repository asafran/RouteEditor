#include "AnimationModel.h"

AnimationModel::AnimationModel(vsg::ref_ptr<route::AnimatedObject> model, QObject *parent)
    : QAbstractListModel{parent}
    , _model(model)
{

}

AnimationModel::~AnimationModel()
{

}

void AnimationModel::setModel(vsg::ref_ptr<route::AnimatedObject> model)
{
    beginResetModel();
    _model = model;
    endResetModel();
}

QModelIndex AnimationModel::addAnimation(QString key, vsg::ref_ptr<Animation> animation)
{
    beginResetModel();
    auto pos = _model->animations.insert({key.toStdString(), animation});
    endResetModel();

    return index(std::distance(_model->animations.begin(), pos.first));
}

void AnimationModel::addBase(vsg::ref_ptr<vsg::Node> base)
{
    //_model->addChild(base);
}

void AnimationModel::remove(const QModelIndex &index)
{
    Q_ASSERT(index.row() < _model->animations.size());
    auto it = std::next(_model->animations.cbegin(), index.row());

    beginResetModel();
    _model->animations.erase(it);
    endResetModel();
}

Animation *AnimationModel::animationIndex(const QModelIndex &index) const
{
    Q_ASSERT(index.row() < _model->animations.size());
    auto it = std::next(_model->animations.cbegin(), index.row());
    return it->second;
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
