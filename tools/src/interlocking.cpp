#include "interlocking.h"

namespace signalling
{
    void JunctionCommand::assemble()
    {
        j->setState(hint);
    }

    void JunctionCommand::disassemble()
    {
        j->setState(false);
    }
    void JunctionCommand::read(vsg::Input &input)
    {
        Object::read(input);

        input.read("junction", j);
        input.read("hint", hint);
    }

    void JunctionCommand::write(vsg::Output &output) const
    {
        Object::write(output);

        output.write("junction", j);
        output.write("hint", hint);
    }

    //-----------------------------------------------------

    void SignalCommand::assemble()
    {
        sig->setState(onHint);
    }

    void SignalCommand::disassemble()
    {
        sig->setState(offHint);
    }

    void SignalCommand::read(vsg::Input &input)
    {
        Object::read(input);

        input.read("signal", sig);
        input.read("onHint", onHint);
        input.read("offHint", offHint);
    }

    void SignalCommand::write(vsg::Output &output) const
    {
        Object::write(output);

        output.write("signal", sig);
        output.write("onHint", onHint);
        output.write("offHint", offHint);
    }

    //-----------------------------------------------------

    Route::Route() : QObject(nullptr)
      , vsg::Inherit<vsg::Object, Route>()
    {

    }

    void Route::read(vsg::Input &input)
    {
        Object::read(input);

        input.read("commands", commands);
        input.read("trajectories", trajs);
    }

    void Route::write(vsg::Output &output) const
    {
        Object::write(output);

        output.write("commands", commands);
        output.write("trajectories", trajs);
    }

    void Route::passed(int ref)
    {
        _count += ref;
        if(_count == 0)
            disassemble();
    }

    bool Route::disassemble()
    {
        for(const auto& trj : trajs)
            if(trj->isBusy())
                return false;
        for(auto& cmd : commands) cmd->disassemble();
        return true;
    }
    bool Route::assemble()
    {
        for(const auto& trj : trajs)
            if(trj->isBusy())
                return false;
        for(auto& cmd : commands) cmd->assemble();
        return true;
    }

    bool Route::onlyJcts()
    {
        for(const auto& trj : trajs)
            if(trj->isBusy())
                return false;
        for(auto& cmd : commands)
            if(cmd->is_compatible(typeid(JunctionCommand)))
                cmd->assemble();
        return true;
    }

    //-----------------------------------------------------

    void Routes::read(vsg::Input &input)
    {
        Object::read(input);

        uint32_t numStations = input.readValue<uint32_t>("NumRoutes");
        routes.clear();
        for (uint32_t i = 0; i < numStations; ++i)
        {
            vsg::ref_ptr<signalling::Signal> signal;
            vsg::ref_ptr<signalling::Route> route;
            input.read("Signal", signal);
            input.read("Route", route);
            if (route) routes.insert_or_assign(signal, route);
        }
    }

    void Routes::write(vsg::Output &output) const
    {
        Object::write(output);

        output.writeValue<uint32_t>("NumRoutes", routes.size());
        for (const auto& route : routes)
        {
            output.write("Signal", vsg::ref_ptr<signalling::Signal>(route.first));
            output.write("Route", route.second);
        }
    }

    //-----------------------------------------------------

    void Station::read(vsg::Input &input)
    {
        Object::read(input);

        uint32_t numSignals = input.readValue<uint32_t>("NumSignals");
        rsignals.clear();
        for (uint32_t i = 0; i < numSignals; ++i)
        {
            vsg::ref_ptr<signalling::Signal> signal;
            vsg::ref_ptr<signalling::Routes> routes;
            input.read("Signal", signal);
            input.read("Routes", routes);
            if (routes) rsignals.insert_or_assign(signal, routes);
        }

        input.read("shunt", shunt);
    }

    void Station::write(vsg::Output &output) const
    {
        Object::write(output);

        output.writeValue<uint32_t>("NumSignals", rsignals.size());
        for (const auto& signal : rsignals)
        {
            output.write("Signal", vsg::ref_ptr<signalling::Signal>(signal.first));
            output.write("Routes", signal.second);
        }

        output.write("shunt", shunt);
    }

    //-----------------------------------------------------

    void RouteCommand::assemble()
    {
        rt->assemble();
    }

    void RouteCommand::disassemble()
    {
        rt->disassemble();
    }

    void RouteCommand::read(vsg::Input &input)
    {
        Object::read(input);

        input.read("route", rt);
    }

    void RouteCommand::write(vsg::Output &output) const
    {
        Object::write(output);

        output.write("route", rt);
    }

}
