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

    }

    void Signal::write(vsg::Output &output) const
    {
        SceneObject::write(output);

    }

    void Signal::ref()
    {
        vcount++;
    }

    void Signal::unref()
    {
        vcount--;
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

    void AutoBlockSignal3::setFwd(AutoBlockSignal3 *_front)
    {

    }

    void AutoBlockSignal3::update()
    {

    }



}
