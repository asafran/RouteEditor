#include "signal.h"
#include "LambdaVisitor.h"
#include <QPauseAnimation>
#include "QFuture"

namespace signalling
{
    Signal::Signal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, int pause, int duration)
        : QObject(nullptr), vsg::Inherit<SceneObject, Signal>(loaded, box), _pause(pause), _duration(duration) {}

    Signal::Signal() : QObject(nullptr), vsg::Inherit<SceneObject, Signal>() {}

    Signal::~Signal() {}

    void Signal::read(vsg::Input &input)
    {
        SceneObject::read(input);

        input.read("station", station);
        input.read("intensity", _intensity);
        input.read("pause", _pause);
        input.read("duration", _duration);
    }

    void Signal::write(vsg::Output &output) const
    {
        SceneObject::write(output);

        output.write("station", station);
        output.write("intensity", _intensity);
        output.write("pause", _pause);
        output.write("duration", _duration);
    }

    void Signal::setState(State hint)
    {
        if(_state == hint)
            return;

        _group.clear();

        if(auto off = getAnim(_state, _front); off)
        {
            off->setDirection(QAbstractAnimation::Backward);
            off->start(QAbstractAnimation::DeleteWhenStopped);
            //_group.addAnimation(off);
            _group.addPause(_pause);
        }
        if(auto on = getAnim(hint, _front); on)
        {
            on->setDirection(QAbstractAnimation::Forward);
            _group.addAnimation(on);
        }
        _group.start();

        _state = hint;

        if((_front == V0 || _front == Off) && hint == Vy)
            emit sendState(VyV0);
        else
            emit sendState(hint);

    }

    void Signal::setFwdState(State front)
    {
        if(_front == front)
            return;

        _group.clear();

        if(auto off = getAnim(_state, _front); off)
        {
            off->setDirection(QAbstractAnimation::Backward);
            off->start(QAbstractAnimation::DeleteWhenStopped);
            //_group.addAnimation(off);
            _group.addPause(_pause);
        }
        if(auto on = getAnim(_state, front); on)
        {
            on->setDirection(QAbstractAnimation::Forward);
            _group.addAnimation(on);
        }
        _group.start();

        _front = front;

        if((_front == V0 || _front == Off) && _state == Vy)
            emit sendState(VyV0);
    }

    void Signal::Ref(int c)
    {
        _vcount += c;
        Q_ASSERT(_vcount >= 0);
    }

    QAbstractAnimation *Signal::getAnim(State state, State front)
    {
        return nullptr;
    }

    void* Signal::operator new(std::size_t count, void* ptr)
    {
        return ::operator new(count, ptr);
    }

    void* Signal::operator new(std::size_t count)
    {
        return vsg::allocate(count, vsg::ALLOCATOR_AFFINITY_OBJECTS);
    }

    void Signal::operator delete(void* ptr)
    {
        vsg::deallocate(ptr);
    }

    //------------------------------------------------------------------------------

    ShSignal::ShSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, int pause, int duration)
        : vsg::Inherit<Signal, ShSignal>(loaded, box, pause, duration)
    {
        auto findLights = [this](vsg::Light& l) mutable
        {
            _intensity = std::max(_intensity, l.intensity);
            l.intensity = 0.0f;
            if(l.name == "S_0")
                _s = vsg::ref_ptr<vsg::Light>(&l);
            else if(l.name == "W_0")
                _w = vsg::ref_ptr<vsg::Light>(&l);
        };
        LambdaVisitor<decltype (findLights), vsg::Light> lv(findLights);
        loaded->accept(lv);

        if(!_s)
            throw SigException{"S_0"};
        else if(!_w)
            throw SigException{"Y_0"};

        setState(NoSh);
    }

    ShSignal::ShSignal() : vsg::Inherit<Signal, ShSignal>() {  }

    ShSignal::~ShSignal() {}

    void ShSignal::read(vsg::Input &input)
    {
        Signal::read(input);

        input.read("S_0", _s);
        input.read("W_0", _w);

        _s->intensity = 0.0f;
        _w->intensity = 0.0f;

        setState(NoSh);

    }

    void ShSignal::write(vsg::Output &output) const
    {
        Signal::write(output);

        output.write("R_0", _s);
        output.write("Y_0", _w);
    }

    void ShSignal::setFwdState(State front)
    {
    }

    void ShSignal::Ref(int c)
    {
        if(c > 0)
            setState(NoSh);
    }

    QAbstractAnimation* ShSignal::getAnim(State state, State front)
    {
        Q_UNUSED(front);

        switch (state) {
        case Sh:
            return new LightAnimation(_w, _intensity, _duration);
        case NoSh:
            return new LightAnimation(_s, _intensity, _duration);
        default:
            return nullptr;
        }
    }

    //------------------------------------------------------------------------------

    Sh2Signal::Sh2Signal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, int pause, int duration)
        : vsg::Inherit<ShSignal, Sh2Signal>(loaded, box, pause, duration)
    {
        vsg::ref_ptr<vsg::Light> w1;
        auto findLights = [&w1](vsg::Light& l) mutable
        {
            l.intensity = 0.0f;
            if(l.name == "W_1")
                w1 = vsg::ref_ptr<vsg::Light>(&l);
        };
        LambdaVisitor<decltype (findLights), vsg::Light> lv(findLights);
        loaded->accept(lv);

        if(!w1)
            throw SigException{"W_1"};
    }

    Sh2Signal::Sh2Signal() : vsg::Inherit<ShSignal, Sh2Signal>() {}

    Sh2Signal::~Sh2Signal() {}

    void Sh2Signal::read(vsg::Input &input)
    {
        ShSignal::read(input);

        input.read("W_1", _w1);

        _w1->intensity = 0.0f;
    }

    void Sh2Signal::write(vsg::Output &output) const
    {
        ShSignal::write(output);

        output.write("W_1", _w1);
    }

    QAbstractAnimation *Sh2Signal::getAnim(State state, State front)
    {
        switch (state) {
        case Sh:
            return new LightAnimation(_w, _intensity, _duration);
        case Sh2:
            return new LightAnimation(_w1, _intensity, _duration);
        case NoSh:
            return new LightAnimation(_s, _intensity, _duration);
        default:
            return nullptr;
        }
    }

    //------------------------------------------------------------------------------

    AutoBlockSignal::AutoBlockSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, int pause, int duration, bool fstate)
        : vsg::Inherit<Signal, AutoBlockSignal>(loaded, box, pause, duration)
        , _fstate(fstate)
    {
        auto findLights = [this](vsg::Light& l) mutable
        {
            _intensity = std::max(_intensity, l.intensity);
            l.intensity = 0.0f;
            if(l.name == "R_0")
                _r = vsg::ref_ptr<vsg::Light>(&l);
            else if(l.name == "Y_0")
                _y = vsg::ref_ptr<vsg::Light>(&l);
            else if(l.name == "G_0")
                _g = vsg::ref_ptr<vsg::Light>(&l);
        };
        LambdaVisitor<decltype (findLights), vsg::Light> lv(findLights);
        loaded->accept(lv);

        if(!_r)
            throw SigException{"R_0"};
        else if(!_y)
            throw SigException{"Y_0"};
        else if(!_g)
            throw SigException{"G_0"};
    }

    AutoBlockSignal::AutoBlockSignal() : vsg::Inherit<Signal, AutoBlockSignal>() {}

    AutoBlockSignal::~AutoBlockSignal()
    {

    }

    void AutoBlockSignal::read(vsg::Input &input)
    {
        Signal::read(input);

        input.read("R_0", _r);
        input.read("Y_0", _y);
        input.read("G_0", _g);

        _r->intensity = 0.0f;
        _y->intensity = 0.0f;
        _g->intensity = 0.0f;
    }

    void AutoBlockSignal::write(vsg::Output &output) const
    {
        Signal::write(output);

        output.writeObject("R_0", _r);
        output.writeObject("Y_0", _y);
        output.writeObject("G_0", _g);
    }

    void AutoBlockSignal::Ref(int c)
    {
        Signal::Ref(c);

        if(_state != Off)
        {
            if(_vcount > 0)
                setState(V0);
            else
                setState(Vy);
        }
    }

    QAbstractAnimation* AutoBlockSignal::getAnim(State state, State front)
    {
        switch (state) {
        case Vy:
            switch (front) {
            case Vy:
                return new LightAnimation(_g, _intensity, _duration);
            case V0:
                return new LightAnimation(_y, _intensity, _duration);;
            case VyV0:
            {
                if(!_fstate)
                    return new LightAnimation(_g, _intensity, _duration);

                QParallelAnimationGroup *group = new QParallelAnimationGroup;
                group->addAnimation(new LightAnimation(_y, _intensity, _duration));
                group->addAnimation(new LightAnimation(_g, _intensity, _duration));

                return group;
            }
            default:
                return new LightAnimation(_y, _intensity, _duration);
            }
        case VyV0:
            return new LightAnimation(_y, _intensity, _duration);
        case V0:
            return new LightAnimation(_r, _intensity, _duration);
        default:
            return nullptr;
        }
    }

    //------------------------------------------------------------------------------

    StRepSignal::StRepSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, int pause, int duration, int pauseOn, int pauseOff, bool fstate)
        : vsg::Inherit<AutoBlockSignal, StRepSignal>(loaded, box, pause, duration, fstate)
        , _pauseOn(pauseOn)
        , _pauseOff(pauseOff)
    {
    }

    StRepSignal::StRepSignal()
        : vsg::Inherit<AutoBlockSignal, StRepSignal>() {}

    StRepSignal::~StRepSignal() {}

    void StRepSignal::read(vsg::Input &input)
    {
        AutoBlockSignal::read(input);

        input.read("pauseOn", _pauseOn);
        input.read("pauseOff", _pauseOff);
    }

    void StRepSignal::write(vsg::Output &output) const
    {
        AutoBlockSignal::write(output);

        output.write("pauseOn", _pauseOn);
        output.write("pauseOff", _pauseOff);
    }

    QAbstractAnimation* StRepSignal::getAnim(State state, State front)
    {
        if(state == Vy)
        {
            if(state == _state && front == _front)
                switch (front) {
                case V1:
                    return nullptr;
                case V2:
                    return nullptr;
                default:
                    break;
                }
            else
                switch (state) {
                case V1:
                {
                    auto yloop = new QSequentialAnimationGroup(this);

                    auto back = new LightAnimation(_y, _intensity, _duration);
                    back->setDirection(QAbstractAnimation::Backward);

                    yloop->addAnimation(new LightAnimation(_y, _intensity, _duration));
                    yloop->addPause(_pauseOn);
                    yloop->addAnimation(back);
                    yloop->addPause(_pauseOff);

                    yloop->setLoopCount(-1);
                    return nullptr;
                }
                case V2:
                {
                    auto gloop = new QSequentialAnimationGroup(this);

                    auto back = new LightAnimation(_g, _intensity, _duration);
                    back->setDirection(QAbstractAnimation::Backward);

                    gloop->addAnimation(new LightAnimation(_g, _intensity, _duration));
                    gloop->addPause(_pauseOn);
                    gloop->addAnimation(back);
                    gloop->addPause(_pauseOff);

                    gloop->setLoopCount(-1);
                    return nullptr;
                }
                default:
                    break;
                }
        }
        return AutoBlockSignal::getAnim(state, front);
    }

    //------------------------------------------------------------------------------

    RouteSignal::RouteSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, int pause, int duration, int pauseOn, int pauseOff, bool fstate)
        : vsg::Inherit<StRepSignal, RouteSignal>(loaded, box, pause, duration, pauseOn, pauseOff, fstate)
    {
        auto findLights = [this](vsg::Light& l) mutable
        {
            _intensity = std::max(_intensity, l.intensity);
            l.intensity = 0.0f;
            if(l.name == "W_0")
                _w = vsg::ref_ptr<vsg::Light>(&l);
            else if(l.name == "Y_1")
                _y1 = vsg::ref_ptr<vsg::Light>(&l);
        };
        LambdaVisitor<decltype (findLights), vsg::Light> lv(findLights);
        loaded->accept(lv);

        if(!_w)
            throw SigException{"W_0"};
        else if(!_y1)
            throw SigException{"Y_1"};
    }

    RouteSignal::RouteSignal() : vsg::Inherit<StRepSignal, RouteSignal>() {}

    RouteSignal::~RouteSignal() {}

    void RouteSignal::read(vsg::Input &input)
    {
        StRepSignal::read(input);

        input.read("Y_1", _y1);
        input.read("W_0", _w);

        _y1->intensity = 0.0f;
        _w->intensity = 0.0f;
    }

    void RouteSignal::write(vsg::Output &output) const
    {
        AutoBlockSignal::write(output);

        output.write("Y_1", _y1);
        output.write("W_0", _w);
    }

    QAbstractAnimation* RouteSignal::getAnim(State state, State front)
    {
        auto msig = StRepSignal::getAnim(state, front);

        switch (state) {
        case Sh:
            return new LightAnimation(_w, _intensity, _duration);
        case Meet:
        {
            if(state == _state && front == _front)
                return nullptr;
            else
            {
                auto wloop = new QSequentialAnimationGroup(this);

                auto back = new LightAnimation(_w, _intensity, _duration);
                back->setDirection(QAbstractAnimation::Backward);

                wloop->addAnimation(new LightAnimation(_w, _intensity, _duration));
                wloop->addPause(_pauseOn);
                wloop->addAnimation(back);
                wloop->addPause(_pauseOff);

                wloop->setLoopCount(-1);
            }
        }
        case V1:
        {
            QParallelAnimationGroup *group = new QParallelAnimationGroup;
            group->addAnimation(new LightAnimation(_y1, _intensity, _duration));

            if(msig)
                group->addAnimation(msig);

            return group;
        }
        default:
            return msig;
        }
    }

    //------------------------------------------------------------------------------

    RouteV2Signal::RouteV2Signal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, int pause, int duration, int pauseOn, int pauseOff, bool fstate)
        : vsg::Inherit<RouteSignal, RouteV2Signal>(loaded, box, pause, duration, pauseOn, pauseOff, fstate)
    {
        auto findLights = [this](vsg::Light& l) mutable
        {
            l.intensity = 0.0f;
            if(l.name == "LINE_0")
                _gline = vsg::ref_ptr<vsg::Light>(&l);
        };
        LambdaVisitor<decltype (findLights), vsg::Light> lv(findLights);
        loaded->accept(lv);

        if(!_gline)
            throw SigException{"LINE_0"};
    }

    RouteV2Signal::RouteV2Signal() : vsg::Inherit<RouteSignal, RouteV2Signal>() {}

    RouteV2Signal::~RouteV2Signal() {}

    void RouteV2Signal::read(vsg::Input &input)
    {
        RouteSignal::read(input);

        input.read("LINE_0", _gline);

        _gline->intensity = 0.0f;
    }

    void RouteV2Signal::write(vsg::Output &output) const
    {
        RouteSignal::write(output);

        output.write("LINE_0", _gline);
    }

    QAbstractAnimation *RouteV2Signal::getAnim(State state, State front)
    {
        auto msig = RouteSignal::getAnim(state, front);

        if(state != V2)
            return msig;

        QParallelAnimationGroup *group = new QParallelAnimationGroup;
        group->addAnimation(new LightAnimation(_gline, _intensity, _duration));

        if(msig)
            group->addAnimation(msig);

        return group;
    }

    //------------------------------------------------------------------------------
/*
    ExitSignal::ExitSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate)
        : vsg::Inherit<AutoBlockSignal, ExitSignal>(loaded, box, fstate) {}

    ExitSignal::ExitSignal() {}

    ExitSignal::~ExitSignal() {}

    void ExitSignal::read(vsg::Input &input)
    {

    }

    void ExitSignal::write(vsg::Output &output) const
    {

    }

    void ExitSignal::update()
    {

    }
*/
}

