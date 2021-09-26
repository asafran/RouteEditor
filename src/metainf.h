#ifndef METAINF_H
#define METAINF_H

#include <QString>
#include <vsg/core/Inherit.h>

enum ObjectType
{
    ObjectGroup,
    RailsGroup,
    BinGroup,
    ShaderGroup,
    Object,
    Trajectory,
    Track,
    Trackside
};
/*
struct MetaInfo : vsg::Object
{
    MetaInfo(QString in_name, ObjectType in_type)
        : name(in_name)
        , type(in_type)
    {
    }
    MetaInfo()
    {
    }
    MetaInfo(const MetaInfo &other)
        : name(other.name)
        , type(other.type)
    {
    }
    QString name;
    ObjectType type;
};
*/
#endif // METAINF_H
