#include "signal.h"
#include "LambdaVisitor.h"

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

    void Signal::Ref(int c)
    {
        _vcount += c;
        Q_ASSERT(_vcount >= 0);
        update();
    }

    AutoBlockSignal3::AutoBlockSignal3(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate)
        : vsg::Inherit<Signal, AutoBlockSignal3>(loaded, box)
        , _fstate(fstate)
    {
        _signals.fill(vsg::ref_ptr<vsg::Light>());
        auto findLights = [this](vsg::Light& l) mutable
        {
            l.intensity = 0.0f;
            if(l.name == "R_0")
                _signals[R] = vsg::ref_ptr<vsg::Light>(&l);
            else if(l.name == "Y_0")
                _signals[Y] = vsg::ref_ptr<vsg::Light>(&l);
            else if(l.name == "G_0")
                _signals[G] = vsg::ref_ptr<vsg::Light>(&l);
        };
        LambdaVisitor<decltype (findLights), vsg::Light> lv(findLights);
        loaded->accept(lv);
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

    void AutoBlockSignal3::update()
    {
        State state = CLOSED;
        if(_vcount == 0)
        {
            switch(_front)
            {
            case route::PREPARE_CLOSED:
                if(_fstate)
                    state = PREPARE_PREAPRE;
                else
                    state = OPENED;
                break;
            case route::CLOSED:
                state = PREPARE_CLOSED;
                break;
            case route::RESTR:
                if(_repeater)
                    state = PREPARE_RESTR;
                else
                    state = OPENED;
                break;
            default:
                state = OPENED;
                break;
            }
        }
        if(_state != state)
        {
            _signals.at(R)->intensity = (state == CLOSED) ? _intensity : 0.0f;
            _signals.at(G)->intensity = (state == OPENED) ? _intensity : 0.0f;
            _signals.at(Y)->intensity = (state == PREPARE_CLOSED) ? _intensity : 0.0f;
            _state = state;
            emit sendState(state);
        }
    }


    void AutoBlockSignal3::setFwdState(route::State state)
    {
        _front = state;
        update();
    }



}
