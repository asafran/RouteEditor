#include "signal.h"

namespace route
{
    Signal::Signal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box)
        : QObject(nullptr), vsg::Inherit<SceneObject, Signal>(loaded, box) {}

    Signal::Signal() : QObject(nullptr) {}

    Signal::~Signal() {}

    void Signal::read(vsg::Input &input)
    {
        SceneObject::read(input);

        input.read("fwdTrajectories", _fwd);
        input.read("bwdTrajectories", _bwd);
    }

    void Signal::write(vsg::Output &output) const
    {
        SceneObject::write(output);

        output.write("fwdTrajectories", _fwd);
        output.write("bwdTrajectories", _bwd);

        for(const auto &trj : _fwd)
        {
            connect(this, &Signal::sendCode, trj.get(), &Trajectory::sendCode);
            connect(trj.get(), &Trajectory::update, this, &Signal::update);
        }
    }

    AutoBlockSignal3::AutoBlockSignal3(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box)
        : vsg::Inherit<Signal, AutoBlockSignal3>(loaded, box)
    {

    }

    AutoBlockSignal3::AutoBlockSignal3()
    {

    }

    AutoBlockSignal3::~AutoBlockSignal3()
    {

    }

    void AutoBlockSignal3::read(vsg::Input &input)
    {
        Signal::read(input);

        for (auto& child : _signals)
        {
            input.readObject("Signal", child);
        }

        for(const auto &trj : _fwd)
        {
            connect(this, &Signal::sendCode, trj.get(), &Trajectory::sendCode);
            connect(trj.get(), &Trajectory::update, this, &Signal::update);
        }

        input.read("frontSignal", _front);
    }

    void AutoBlockSignal3::write(vsg::Output &output) const
    {
        Signal::write(output);

        for (const auto& child : _signals)
        {
            output.writeObject("Signal", child.get());
        }

        output.write("frontSignal", _front);
    }

    void AutoBlockSignal3::update()
    {
        for(const auto& trj : _fwd)
        {
            if(trj->isBusy())
            {
                _state = R;

            }
        }
    }



}
