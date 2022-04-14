#include "signal.h"
#include "LambdaVisitor.h"
#include <QPauseAnimation>

namespace signalling
{
    Signal::Signal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box)
        : QObject(nullptr), vsg::Inherit<SceneObject, Signal>(loaded, box) {}

    Signal::Signal() : QObject(nullptr), vsg::Inherit<SceneObject, Signal>() {}

    Signal::~Signal() {}

    void Signal::read(vsg::Input &input)
    {
        SceneObject::read(input);

        input.read("station", station);
        input.read("intensity", _intensity);
    }

    void Signal::write(vsg::Output &output) const
    {
        SceneObject::write(output);

        output.write("station", station);
        output.write("intensity", _intensity);
    }

    void Signal::setState(State hint)
    {
        if(_state == hint)
            return;

        clear();

        auto *group = new QSequentialAnimationGroup;
        if(auto off = getAnim(_state, _front); off)
        {
            off->setDirection(QAbstractAnimation::Backward);
            group->addAnimation(off);
        }
        if(auto on = getAnim(hint, _front); on)
        {
            on->setDirection(QAbstractAnimation::Forward);
            group->addAnimation(on);
        }
        group->start(QAbstractAnimation::KeepWhenStopped);
        _state = hint;

        if(_front == V0 && hint == Vy)
            emit sendState(VyV0);
        else
            emit sendState(hint);

    }

    void Signal::setFwdState(State front)
    {
        if(_front == front)
            return;

        clear();

        if(auto off = getAnim(_state, _front); off)
        {
            off->setDirection(QAbstractAnimation::Backward);
            _group->addAnimation(off);
        }
        if(auto on = getAnim(_state, front); on)
        {
            on->setDirection(QAbstractAnimation::Forward);
            _group->addAnimation(on);
        }
        _group->start(QAbstractAnimation::KeepWhenStopped);
        _front = front;


        if(_front == V0 && _state == Vy)
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

    void Signal::clear()
    {
        while(_group->animationCount() > 0)
        {
            if(auto anim = dynamic_cast<QAnimationGroup*>(_group->takeAnimation(0)); anim)
            {
                while(anim->animationCount() > 0)
                    anim->takeAnimation(0);
                delete anim;
            }
        }
    }

    //------------------------------------------------------------------------------

    ShSignal::ShSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box)
        : vsg::Inherit<Signal, ShSignal>(loaded, box)
    {
        vsg::ref_ptr<vsg::Light> s;
        vsg::ref_ptr<vsg::Light> w;
        auto findLights = [&s, &w](vsg::Light& l) mutable
        {
            l.intensity = 0.0f;
            if(l.name == "S_0")
                s = vsg::ref_ptr<vsg::Light>(&l);
            else if(l.name == "W_0")
                w = vsg::ref_ptr<vsg::Light>(&l);
        };
        LambdaVisitor<decltype (findLights), vsg::Light> lv(findLights);
        loaded->accept(lv);

        if(!s)
            throw SigException{"S_0"};
        else if(!w)
            throw SigException{"Y_0"};

        initSigs(s, w);
    }

    ShSignal::ShSignal() : vsg::Inherit<Signal, ShSignal>() { }

    ShSignal::~ShSignal() {}

    void ShSignal::read(vsg::Input &input)
    {
        Signal::read(input);

        vsg::ref_ptr<vsg::Light> s;
        vsg::ref_ptr<vsg::Light> w;

        input.read("S_0", s);
        input.read("W_0", w);

        initSigs(s, w);
    }

    void ShSignal::write(vsg::Output &output) const
    {
        Signal::write(output);

        output.write("R_0", _sanim->light);
        output.write("Y_0", _wanim->light);
    }

    void ShSignal::setFwdState(State front)
    {
        emit sendState(front);
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
            return _wanim;
        case NoSh:
            return _sanim;
        default:
            return nullptr;
        }
    }

    void ShSignal::initSigs(vsg::ref_ptr<vsg::Light> s, vsg::ref_ptr<vsg::Light> w)
    {
        _state = NoSh;

        _sanim = new LightAnimation(s, this);
        _sanim->setDuration(1000);
        _sanim->setStartValue(0.0f);
        _sanim->setEndValue(_intensity);

        _wanim = new LightAnimation(w, this);
        _wanim->setDuration(1000);
        _wanim->setStartValue(0.0f);
        _wanim->setEndValue(_intensity);

        _sanim->start();
    }

    //------------------------------------------------------------------------------

    Sh2Signal::Sh2Signal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box)
        : vsg::Inherit<ShSignal, Sh2Signal>(loaded, box)
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

        initSigs(w1);
    }

    Sh2Signal::Sh2Signal() : vsg::Inherit<ShSignal, Sh2Signal>() {}

    Sh2Signal::~Sh2Signal() {}

    void Sh2Signal::read(vsg::Input &input)
    {
        ShSignal::read(input);

        vsg::ref_ptr<vsg::Light> w1;

        input.read("W_1", w1);

        initSigs(w1);
    }

    void Sh2Signal::write(vsg::Output &output) const
    {
        ShSignal::write(output);

        output.write("W_1", _w1anim->light);
    }

    QAbstractAnimation *Sh2Signal::getAnim(State state, State front)
    {
        switch (state) {
        case Sh:
            return _wanim;
        case Sh2:
            return _w1anim;
        case NoSh:
            return _sanim;
        default:
            return nullptr;
        }
    }

    void Sh2Signal::initSigs(vsg::ref_ptr<vsg::Light> w1)
    {
        _w1anim = new LightAnimation(w1, this);
        _w1anim->setDuration(1000);
        _w1anim->setStartValue(0.0f);
        _w1anim->setEndValue(_intensity);
    }

    //------------------------------------------------------------------------------

    AutoBlockSignal::AutoBlockSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate)
        : vsg::Inherit<Signal, AutoBlockSignal>(loaded, box)
        , _fstate(fstate)
    {
        vsg::ref_ptr<vsg::Light> r;
        vsg::ref_ptr<vsg::Light> y;
        vsg::ref_ptr<vsg::Light> g;
        auto findLights = [&r, &y, &g](vsg::Light& l) mutable
        {
            l.intensity = 0.0f;
            if(l.name == "R_0")
                r = vsg::ref_ptr<vsg::Light>(&l);
            else if(l.name == "Y_0")
                y = vsg::ref_ptr<vsg::Light>(&l);
            else if(l.name == "G_0")
                g = vsg::ref_ptr<vsg::Light>(&l);
        };
        LambdaVisitor<decltype (findLights), vsg::Light> lv(findLights);
        loaded->accept(lv);

        if(!r)
            throw SigException{"R_0"};
        else if(!y)
            throw SigException{"Y_0"};
        else if(!g)
            throw SigException{"G_0"};

        initSigs(r, y, g);
    }

    AutoBlockSignal::AutoBlockSignal() : vsg::Inherit<Signal, AutoBlockSignal>() {}

    void AutoBlockSignal::initSigs(vsg::ref_ptr<vsg::Light> r, vsg::ref_ptr<vsg::Light> y, vsg::ref_ptr<vsg::Light> g)
    {
        _ranim = new LightAnimation(r, this);
        _ranim->setDuration(1000);
        _ranim->setStartValue(0.0f);
        _ranim->setEndValue(_intensity);

        _yanim = new LightAnimation(y, this);
        _yanim->setDuration(1000);
        _yanim->setStartValue(0.0f);
        _yanim->setEndValue(_intensity);

        _ganim = new LightAnimation(g, this);
        _ganim->setDuration(1000);
        _ganim->setStartValue(0.0f);
        _ganim->setEndValue(_intensity);
    }

    AutoBlockSignal::~AutoBlockSignal()
    {

    }

    void AutoBlockSignal::read(vsg::Input &input)
    {
        Signal::read(input);

        vsg::ref_ptr<vsg::Light> r;
        vsg::ref_ptr<vsg::Light> y;
        vsg::ref_ptr<vsg::Light> g;

        input.read("R_0", r);
        input.read("Y_0", y);
        input.read("G_0", g);

        initSigs(r, y, g);
    }

    void AutoBlockSignal::write(vsg::Output &output) const
    {
        Signal::write(output);

        output.writeObject("R_0", _ranim->light);
        output.writeObject("Y_0", _yanim->light);
        output.writeObject("G_0", _ganim->light);
    }

    void AutoBlockSignal::Ref(int c)
    {
        Signal::Ref(c);

        if(_vcount > 0)
            setState(V0);
        else
            setState(Vy);
    }

    QAbstractAnimation* AutoBlockSignal::getAnim(State state, State front)
    {
        switch (state) {
        case Vy:
            switch (front) {
            case Vy:
                return _ganim;
            case V0:
                return _yanim;
            case VyV0:
            {
                if(!_fstate)
                    return _ganim;

                QParallelAnimationGroup *group = new QParallelAnimationGroup;
                group->addAnimation(_yanim);
                group->addAnimation(_ganim);

                return group;
            }
            default:
                return _yanim;
            }
        case VyV0:
            return _yanim;
        case V0:
            return _ranim;
        default:
            return nullptr;
        }
    }

    //------------------------------------------------------------------------------

    StRepSignal::StRepSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate)
        : vsg::Inherit<AutoBlockSignal, StRepSignal>(loaded, box, fstate)
    {
        initSigs();
    }

    StRepSignal::StRepSignal()
        : vsg::Inherit<AutoBlockSignal, StRepSignal>() {}

    StRepSignal::~StRepSignal() {}

    void StRepSignal::read(vsg::Input &input)
    {
        AutoBlockSignal::read(input);
        initSigs();
    }

    QAbstractAnimation* StRepSignal::getAnim(State state, State front)
    {
        if(state == Vy)
        {
            if(state == _state && front == _front)
                switch (front) {
                case V1:
                {
                    _yloop->stop();
                    return nullptr;
                }
                case V2:
                {
                    _gloop->stop();
                    return nullptr;
                }
                default:
                    break;
                }
            else
                switch (state) {
                case V1:
                {
                    _yloop->start();
                    return nullptr;
                }
                case V2:
                {
                    _gloop->start();
                    return nullptr;
                }
                default:
                    break;
                }
        }
        return AutoBlockSignal::getAnim(state, front);
    }

    void StRepSignal::initSigs()
    {
        _yloop = new QSequentialAnimationGroup(this);

        auto back = new LightAnimation(_yanim->light, _yloop);
        back->setDuration(1000);
        back->setStartValue(_intensity);
        back->setEndValue(0.0f);

        auto pause = new QPauseAnimation(_yloop);
        pause->setDuration(500);

        _yloop->addAnimation(_yanim);
        _yloop->addAnimation(pause);
        _yloop->addAnimation(back);

        _yloop->setLoopCount(-1);

        _gloop = new QSequentialAnimationGroup(this);

        back = new LightAnimation(_ganim->light, _gloop);
        back->setDuration(1000);
        back->setStartValue(_intensity);
        back->setEndValue(0.0f);

        _gloop->addAnimation(_yanim);
        _gloop->addAnimation(pause);
        _gloop->addAnimation(back);

        _gloop->setLoopCount(-1);
    }

    //------------------------------------------------------------------------------

    RouteSignal::RouteSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate)
        : vsg::Inherit<StRepSignal, RouteSignal>(loaded, box, fstate)
    {
        vsg::ref_ptr<vsg::Light> y1;
        vsg::ref_ptr<vsg::Light> w;
        auto findLights = [&w, &y1](vsg::Light& l) mutable
        {
            l.intensity = 0.0f;
            if(l.name == "W_0")
                w = vsg::ref_ptr<vsg::Light>(&l);
            else if(l.name == "Y_1")
                y1 = vsg::ref_ptr<vsg::Light>(&l);
        };
        LambdaVisitor<decltype (findLights), vsg::Light> lv(findLights);
        loaded->accept(lv);

        if(!w)
            throw SigException{"W_0"};
        else if(!y1)
            throw SigException{"Y_1"};

        initSigs(y1, w);
    }

    RouteSignal::RouteSignal() : vsg::Inherit<StRepSignal, RouteSignal>() {}

    void RouteSignal::initSigs(vsg::ref_ptr<vsg::Light> y1, vsg::ref_ptr<vsg::Light> w)
    {
        _wanim = new LightAnimation(w, this);
        _wanim->setDuration(1000);
        _wanim->setStartValue(0.0f);
        _wanim->setEndValue(_intensity);

        _y1anim = new LightAnimation(y1, this);
        _y1anim->setDuration(1000);
        _y1anim->setStartValue(0.0f);
        _y1anim->setEndValue(_intensity);

        _wloop = new QSequentialAnimationGroup(this);

        auto back = new LightAnimation(w, _wloop);
        back->setDuration(1000);
        back->setStartValue(_intensity);
        back->setEndValue(0.0f);

        auto pause = new QPauseAnimation(_wloop);
        pause->setDuration(500);

        _wloop->addAnimation(_wanim);
        _wloop->addAnimation(pause);
        _wloop->addAnimation(back);

        _wloop->setLoopCount(-1);
    }

    RouteSignal::~RouteSignal() {}

    void RouteSignal::read(vsg::Input &input)
    {
        StRepSignal::read(input);

        vsg::ref_ptr<vsg::Light> y1;
        vsg::ref_ptr<vsg::Light> w;

        input.read("Y_1", y1);
        input.read("W_0", w);

        initSigs(y1, w);
    }

    void RouteSignal::write(vsg::Output &output) const
    {
        AutoBlockSignal::write(output);

        output.write("Y_1", _y1anim->light);
        output.write("W_0", _wanim->light);
    }

    QAbstractAnimation* RouteSignal::getAnim(State state, State front)
    {
        auto msig = StRepSignal::getAnim(state, front);

        switch (state) {
        case Sh:
            return _wanim;
        case Meet:
        {
            if(state == _state && front == _front)
                _wloop->stop();
            else
                _wloop->start();
        }
        case V1:
        {
            QParallelAnimationGroup *group = new QParallelAnimationGroup;
            group->addAnimation(_y1anim);

            if(msig)
                group->addAnimation(msig);

            return group;
        }
        default:
            return msig;
        }
    }

    //------------------------------------------------------------------------------

    RouteV2Signal::RouteV2Signal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate)
        : vsg::Inherit<RouteSignal, RouteV2Signal>(loaded, box, fstate)
    {
        vsg::ref_ptr<vsg::Light> gline;
        auto findLights = [&gline](vsg::Light& l) mutable
        {
            l.intensity = 0.0f;
            if(l.name == "LINE_0")
                gline = vsg::ref_ptr<vsg::Light>(&l);
        };
        LambdaVisitor<decltype (findLights), vsg::Light> lv(findLights);
        loaded->accept(lv);

        if(!gline)
            throw SigException{"LINE_0"};

        initSigs(gline);
    }

    RouteV2Signal::RouteV2Signal() : vsg::Inherit<RouteSignal, RouteV2Signal>() {}

    RouteV2Signal::~RouteV2Signal() {}

    void RouteV2Signal::read(vsg::Input &input)
    {
        RouteSignal::read(input);

        vsg::ref_ptr<vsg::Light> gline;

        input.read("LINE_0", gline);

        initSigs(gline);
    }

    void RouteV2Signal::write(vsg::Output &output) const
    {
        RouteSignal::write(output);

        output.write("LINE_0", _glineanim->light);
    }

    QAbstractAnimation *RouteV2Signal::getAnim(State state, State front)
    {
        auto msig = RouteSignal::getAnim(state, front);

        if(state != V2)
            return msig;

        QParallelAnimationGroup *group = new QParallelAnimationGroup;
        group->addAnimation(_glineanim);

        if(msig)
            group->addAnimation(msig);

        return group;
    }

    void RouteV2Signal::initSigs(vsg::ref_ptr<vsg::Light> line)
    {
        _glineanim = new LightAnimation(line, this);
        _glineanim->setDuration(1000);
        _glineanim->setStartValue(0.0f);
        _glineanim->setEndValue(_intensity);
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

