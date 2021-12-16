#include    "vehicle-controller.h"

#include    "connector.h"

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
Bogie::Bogie(VehicleController *in_parent)
  : parent(in_parent)
  , direction(1)
  , section_coord(0.0)
  , current_traj(Q_NULLPTR)
{

}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
Bogie::~Bogie()
{

}

void Bogie::move(double x)
{
    section_coord += direction * x;

    while (section_coord > (*section)->track->lenght)
    {
        section_coord -= (*section)->track->lenght;
        if(++section == end)
            jumpNext();

    }
    while (section_coord < 0)
    {
        if(section == begin)
            jumpPrev();

        --section;
        section_coord += (*section)->track->lenght;
    }
}

void Bogie::place(Trajectory *traj, double x)
{
    section_coord = 0;
    current_traj->removeBusy(this);
    current_traj = traj;
    current_traj->setBusy(this);
    move(x);
}

void Bogie::jumpNext()
{
    if (current_traj->getFwd() == Q_NULLPTR)
    {
        --section;
        section_coord = (*section)->track->lenght;
        return;
    }

    if (current_traj->isFrontReversed())
    {
        section_coord = (*--current_traj->getFwd()->getEnd(current_traj))->track->lenght - section_coord;
        direction *= -1;
    }

    current_traj->removeBusy(this);
    current_traj->getFwd()->setBusy(this);

    section = current_traj->getFwd()->getBegin(current_traj);
    current_traj = current_traj->getFwd();

}

void Bogie::jumpPrev()
{

    if (current_traj->getBwd() == Q_NULLPTR)
    {
        section_coord = -((*section)->track->lenght);
        ++section;
        return;
    }

    current_traj->removeBusy(this);
    current_traj->getBwd()->setBusy(this);

    section = current_traj->getBwd()->getEnd(current_traj);
    current_traj = current_traj->getBwd();
}

VehicleController::VehicleController()
  : x_cur(0.0)
  , x_prev(0.0)
  , first(this)
  , second(this)
{

}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void VehicleController::setRailwayCoord(double x)
{
    x_prev = x_cur;
    x_cur = x;

    first.move(x);
    second.move(x);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void VehicleController::setInitCurrentTraj(Trajectory *traj, double x)
{
    first.place(traj, x);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
vsg::dvec3 Bogie::getPosition() const
{
    return getTransform() * vsg::dvec3();
}

vsg::dmat4 Bogie::getTransform() const
{
    return (*section)->world(section_coord);
}
