#ifndef     VEHICLE_CONTROLLER_H
#define     VEHICLE_CONTROLLER_H

#include    <QObject>
#include    "trajectory.h"

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

class VehicleController;

class Bogie
{
public:
    Bogie(VehicleController *in_parent);

    virtual ~Bogie();

    void move(double  x);

    void place(Trajectory *traj, double x);

    void jumpNext();

    void jumpPrev();

    void setTopologyDirection(int dir) { direction = dir; }

    vsg::dvec3 getPosition() const;

    vsg::dmat4 getTransform() const;

    Trajectory *getCurrentTraj() const { return current_traj; }

    double getTrajCoord() const;

    int getTopologyDirection() const { return direction; }

private:

    /// Направление движения
    int    direction;

    /// Координата, в пределах текущей траектории
    double section_coord;
    Sections::const_iterator begin;
    Sections::const_iterator section;
    Sections::const_iterator end;

    VehicleController *parent;

    Trajectory *current_traj;
};


class VehicleController
{
public:
    VehicleController();

    virtual ~VehicleController();

    void setRailwayCoord(double x);

    void setInitCurrentTraj(Trajectory *traj, double x);

    vsg::dvec3 getPosition() const;

    vsg::dmat4 getTransform() const;

    //Trajectory *getCurrentTraj() const { return current_traj; }

    //double getTrajCoord() const;

    //int getTopologyDirection() const { return direction; }

private:

    /// Предыдущее значение дуговой координаты ПЕ
    double  x_prev;

    /// Текущее значение дуговой координаты ПЕ
    double  x_cur;

    Bogie first;
    Bogie second;


    // Направление движения
    //int    direction;

    // Координата, в пределах текущей траектории
    //double section_coord;
    //Sections::const_iterator section;

    //Trajectory *current_traj;
};

#endif // VEHICLE_CONTROLLER_H
