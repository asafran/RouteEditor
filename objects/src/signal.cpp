#include "signal.h"
#include "LambdaVisitor.h"
#include <QPauseAnimation>

namespace route
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

    void Signal::Ref(int c)
    {
        _vcount += c;
        Q_ASSERT(_vcount >= 0);
        update();
    }

    AutoBlockSignal::AutoBlockSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate)
        : vsg::Inherit<Signal, AutoBlockSignal>(loaded, box)
        , _fstate(fstate)
    {
        //_signals.fill(vsg::ref_ptr<vsg::Light>());
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

    AutoBlockSignal::AutoBlockSignal()
    {

    }

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

        _loop = new QSequentialAnimationGroup(this);

        auto back = new LightAnimation(y, _loop);
        back->setDuration(1000);
        back->setStartValue(_intensity);
        back->setEndValue(0.0f);

        auto pause = new QPauseAnimation(_loop);
        pause->setDuration(500);

        _loop->addAnimation(_yanim);
        _loop->addAnimation(pause);
        _loop->addAnimation(back);

        _loop->setLoopCount(-1);
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

        output.write("frontSignal", _front);
    }

    void AutoBlockSignal::update()
    {
        State state = CLOSED;
        if(_vcount == 0)
        {
            switch(_front)
            {
            case route::PREPARE_CLOSED:
                if(_fstate)
                    state = PREPARE_PREPARE;
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
            switch (_state) {
            case route::RESTR:
                break;
            case route::OPENED:
            {
                _ganim->setDirection(QAbstractAnimation::Backward);
                _ganim->start();
                break;
            }
            case route::PREPARE_CLOSED:
            {
                _yanim->setDirection(QAbstractAnimation::Backward);
                _yanim->start();
                break;
            }
            case route::PREPARE_PREPARE:
            {
                _yanim->setDirection(QAbstractAnimation::Backward);
                //_yanim->start();

                _ganim->setDirection(QAbstractAnimation::Backward);
                //_ganim->start();

                QParallelAnimationGroup *group = new QParallelAnimationGroup;
                group->addAnimation(_yanim);
                group->addAnimation(_ganim);

                group->start(QAbstractAnimation::DeleteWhenStopped);
                break;
            }
            case route::PREPARE_RESTR:
            {
                _loop->stop();
                break;
            }
            case route::CLOSED:
            {
                _ranim->setDirection(QAbstractAnimation::Backward);
                _ranim->start();
                break;
            }
            default:
                break;
            }

            switch (state) {
            case route::OPENED:
            {
                _ganim->setDirection(QAbstractAnimation::Forward);
                _ganim->start();
                break;
            }
            case route::PREPARE_CLOSED:
            {
                _yanim->setDirection(QAbstractAnimation::Forward);
                _yanim->start();
                break;
            }
            case route::PREPARE_PREPARE:
            {
                _yanim->setDirection(QAbstractAnimation::Forward);

                _ganim->setDirection(QAbstractAnimation::Forward);

                QParallelAnimationGroup *group = new QParallelAnimationGroup;
                group->addAnimation(_yanim);
                group->addAnimation(_ganim);

                group->start(QAbstractAnimation::DeleteWhenStopped);
                break;
            }
            case route::PREPARE_RESTR:
            {
                _loop->start();
                break;
            }
            case route::CLOSED:
            {
                _ranim->setDirection(QAbstractAnimation::Forward);
                _ranim->start();
                break;
            }
            default:
                break;
            }
            _state = state;
            emit sendState(state);
        }
    }

    StSignal::StSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate)
        : vsg::Inherit<AutoBlockSignal, StSignal>(loaded, box, fstate) {}


    StSignal::StSignal() {}


    StSignal::~StSignal() {}


    void StSignal::read(vsg::Input &input)
    {

    }


    void StSignal::write(vsg::Output &output) const
    {

    }


    void StSignal::update()
    {

    }


    void StSignal::open(FwdHint hint)
    {
        _restrHint = hint;
        update();
    }


    void StSignal::close()
    {
        _restrHint = CLOSE_SIG;
        update();
    }

    EnterSignal::EnterSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate)
        : vsg::Inherit<StSignal, EnterSignal>(loaded, box, fstate)
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

    EnterSignal::EnterSignal() {}

    void EnterSignal::initSigs(vsg::ref_ptr<vsg::Light> y1, vsg::ref_ptr<vsg::Light> w)
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

        auto back = new LightAnimation(w, _loop);
        back->setDuration(1000);
        back->setStartValue(_intensity);
        back->setEndValue(0.0f);

        auto pause = new QPauseAnimation(_loop);
        pause->setDuration(500);

        _loop->addAnimation(_wanim);
        _loop->addAnimation(pause);
        _loop->addAnimation(back);

        _loop->setLoopCount(-1);
    }

    EnterSignal::~EnterSignal() {}

    void EnterSignal::read(vsg::Input &input)
    {
        AutoBlockSignal::read(input);

        vsg::ref_ptr<vsg::Light> y1;
        vsg::ref_ptr<vsg::Light> w;

        input.read("Y_1", y1);
        input.read("W_0", w);

        initSigs(y1, w);
    }

    void EnterSignal::write(vsg::Output &output) const
    {

    }

    void EnterSignal::update()
    {
        /*
        State state = CLOSED;
        if(_vcount == 0)
        {
            switch(_front)
            {
            case route::PREPARE_CLOSED:
                if(_fstate)
                    state = PREPARE_PREPARE;
                else
                    state = OPENED;
                break;
            case route::CLOSED:
                state = PREPARE_CLOSED;
                break;
            case route::RESTR:
                //if(_repeater)
                    state = PREPARE_RESTR;
                //else
                    //state = OPENED;
                break;
            default:
                state = OPENED;
                break;
            }
        }
        if(_state != state)
        {
            switch (_state) {
            case route::RESTR:
                break;
            case route::OPENED:
            {
                _ganim->setDirection(QAbstractAnimation::Backward);
                _ganim->start();
                break;
            }
            case route::PREPARE_CLOSED:
            {
                _yanim->setDirection(QAbstractAnimation::Backward);
                _yanim->start();
                break;
            }
            case route::PREPARE_PREPARE:
            {
                _yanim->setDirection(QAbstractAnimation::Backward);
                //_yanim->start();

                _ganim->setDirection(QAbstractAnimation::Backward);
                //_ganim->start();

                QParallelAnimationGroup *group = new QParallelAnimationGroup;
                group->addAnimation(_yanim);
                group->addAnimation(_ganim);

                group->start(QAbstractAnimation::DeleteWhenStopped);
                break;
            }
            case route::PREPARE_RESTR:
            {
                _loop->stop();
                break;
            }
            case route::CLOSED:
            {
                _ranim->setDirection(QAbstractAnimation::Backward);
                _ranim->start();
                break;
            }
            default:
                break;
            }

            switch (state) {
            case route::OPENED:
            {
                _ganim->setDirection(QAbstractAnimation::Forward);
                _ganim->start();
                break;
            }
            case route::PREPARE_CLOSED:
            {
                _yanim->setDirection(QAbstractAnimation::Forward);
                _yanim->start();
                break;
            }
            case route::PREPARE_PREPARE:
            {
                _yanim->setDirection(QAbstractAnimation::Forward);

                _ganim->setDirection(QAbstractAnimation::Forward);

                QParallelAnimationGroup *group = new QParallelAnimationGroup;
                group->addAnimation(_yanim);
                group->addAnimation(_ganim);

                group->start(QAbstractAnimation::DeleteWhenStopped);
                break;
            }
            case route::PREPARE_RESTR:
            {
                _loop->start();
                break;
            }
            case route::CLOSED:
            {
                _ranim->setDirection(QAbstractAnimation::Forward);
                _ranim->start();
                break;
            }
            default:
                break;
            }
            _state = state;
            emit sendState(state);
        }
        */
    }

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
