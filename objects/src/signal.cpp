#include "signal.h"
#include "LambdaVisitor.h"
#include <QPauseAnimation>

namespace signalling
{
    Signal::Signal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box)
        : QObject(nullptr), vsg::Inherit<SceneObject, Signal>(loaded, box) {}

    Signal::Signal() : QObject(nullptr) {}

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

    void Signal::update()
    {
        auto state = processState();
        if(_state != state)
        {
            auto *group = new QSequentialAnimationGroup;
            if(auto off = getAnim(_state); off)
            {
                off->setDirection(QAbstractAnimation::Backward);
                group->addAnimation(off);
            }
            if(auto on = getAnim(state); on)
            {
                on->setDirection(QAbstractAnimation::Forward);
                group->addAnimation(on);
            }
            group->start(QAbstractAnimation::DeleteWhenStopped);
            _state = state;
            emit sendState(state);
        }
    }

    void Signal::set(Hint hint)
    {
        _hint = hint;
        update();
    }

    void Signal::Ref(int c)
    {
        _vcount += c;
        Q_ASSERT(_vcount >= 0);
        update();
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

    ShSignal::ShSignal() { _state = NoSh; }

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

    State ShSignal::processState() const
    {
        switch (_hint) {
        case ShH:
            return Sh;
        default:
            return NoSh;
        }
    }

    QAbstractAnimation* ShSignal::getAnim(State state)
    {
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

    State Sh2Signal::processState() const
    {
        switch (_hint) {
        case ShH:
            return Sh;
        case Sh2H:
            return Sh2;
        default:
            return NoSh;
        }
    }

    QAbstractAnimation *Sh2Signal::getAnim(State state)
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

        _ganim->start();
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

        output.write("R_0", _ranim->light);
        output.write("Y_0", _yanim->light);
        output.write("G_0", _ganim->light);
    }

    State AutoBlockSignal::processState() const
    {
        State state = V0;
        if(_vcount == 0)
        {
            switch(_front)
            {
            case VyV0:
                if(_fstate)
                    state = VyVyV0;
                else
                    state = VyVy;
                break;
            case V0:
                state = VyV0;
                break;
            case Sh:
            case Sh2:
            case NoSh:
                state = V0;
            default:
                state = VyVy;
                break;
            }
        }
        return state;
    }

    QAbstractAnimation* AutoBlockSignal::getAnim(State state)
    {
        switch (_state) {
        case VyVy:
            return _ganim;
        case VyV0:
            return _yanim;
        case VyVyV0:
        {
            QParallelAnimationGroup *group = new QParallelAnimationGroup;
            group->addAnimation(_yanim);
            group->addAnimation(_ganim);

            return group;
        }
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

    State StRepSignal::processState() const
    {
        State state = V0;
        if(_vcount == 0)
        {
            switch(_front)
            {
            case V1V0:
            case V1V1:
                state = VyV1;
                break;
            case V2V0:
            case V2V1:
            case V2V2:
                state = VyV2;
                break;
            default:
                return AutoBlockSignal::processState();
            }
        }
        return state;
    }

    QAbstractAnimation* StRepSignal::getAnim(State state)
    {
        if(state == _state)
            switch (state) {
            case V1V1:
            case V2V1:
            case VyV1:
            {
                _yloop->stop();
                return nullptr;
            }
            case V2V2:
            case VyV2:
            {
                _gloop->stop();
                return nullptr;
            }
            default:
                break;
            }
        else
            switch (state) {
            case V1V1:
            case V2V1:
            case VyV1:
            {
                _yloop->start();
                return nullptr;
            }
            case V2V2:
            case VyV2:
            {
                _gloop->start();
                return nullptr;
            }
            default:
                break;
            }
        return AutoBlockSignal::getAnim(state);
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

        _y2anim = new LightAnimation(y1, this);
        _y2anim->setDuration(1000);
        _y2anim->setStartValue(0.0f);
        _y2anim->setEndValue(_intensity);

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
        AutoBlockSignal::read(input);

        vsg::ref_ptr<vsg::Light> y1;
        vsg::ref_ptr<vsg::Light> w;

        input.read("Y_1", y1);
        input.read("W_0", w);

        initSigs(y1, w);
    }

    void RouteSignal::write(vsg::Output &output) const
    {

    }

    State RouteSignal::processState() const
    {
        State state = V0;
        if(_vcount == 0)
        {
            switch(_front)
            {
            case V1V0:
            case V1V1:
                state = VyV1;
                break;
            case V2V0:
            case V2V1:
            case V2V2:
                state = VyV2;
                break;
            default:
                return AutoBlockSignal::processState();
                break;
            }
        }
        return state;
    }

    QAbstractAnimation* RouteSignal::getAnim(State state)
    {
        switch (state) {
        case V1V0:
        {
            QParallelAnimationGroup *group = new QParallelAnimationGroup;
            group->addAnimation(_yanim);
            group->addAnimation(_ganim);

            return group;
        }
        case V2V0:
        {
            QParallelAnimationGroup *group = new QParallelAnimationGroup;
            group->addAnimation(_yanim);
            group->addAnimation(_ganim);

            return group;
        }
        default:
            return nullptr;
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

    }

    void RouteV2Signal::write(vsg::Output &output) const
    {

    }

    State RouteV2Signal::processState() const
    {

    }

    QAbstractAnimation *RouteV2Signal::getAnim(State state)
    {

    }

    void RouteV2Signal::initSigs(vsg::ref_ptr<vsg::Light> line)
    {

    }

    //------------------------------------------------------------------------------

    ExitSignal::ExitSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate)
        : vsg::Inherit<StSignal, ExitSignal>(loaded, box, fstate) {}

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

}
