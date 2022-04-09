#ifndef SIGNAL_H
#define SIGNAL_H

#include "sceneobjects.h"
#include "trajectory.h"
#include "lightanimation.h"
#include <vsg/nodes/Switch.h>
#include <vsg/nodes/Light.h>
#include "Constants.h"
#include <QSequentialAnimationGroup>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

namespace signalling
{
    class SigException
    {
    public:
        QString errL;
    };
    class Signal : public QObject, public vsg::Inherit<route::SceneObject, Signal>
    {
        Q_OBJECT
    public:
        Signal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box);
        Signal();

        virtual ~Signal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void update();

        virtual State processState() const;
        virtual bool applyState(State state);

        void setFwdState(signalling::State state) { _front = state; update(); }
        void Ref(int c);

        std::string station;

    signals:
        void sendCode(signalling::Code code);
        void sendState(signalling::State state);

    protected:
        State _front = V0;

        State _state = VyVy;

        int _vcount = 0;

        float _intensity = 3.0f;

        friend class route::SwitchConnector;
    };

    class AutoBlockSignal : public vsg::Inherit<Signal, AutoBlockSignal>
    {
        Q_OBJECT
    public:
        AutoBlockSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate = false);
        AutoBlockSignal();

        virtual ~AutoBlockSignal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        State processState() const override;
        bool applyState(State state) override;

        //void update() override;

        virtual State getState() { return VyVy; }
    protected:

        LightAnimation *_ranim;
        LightAnimation *_yanim;
        LightAnimation *_ganim;

        QSequentialAnimationGroup *_loop;

        bool _fstate = false;

    private:
        void initSigs(vsg::ref_ptr<vsg::Light> r, vsg::ref_ptr<vsg::Light> y, vsg::ref_ptr<vsg::Light> g);
    };

    class StRepSignal : public vsg::Inherit<AutoBlockSignal, StRepSignal>
    {
        Q_OBJECT
    public:
        StRepSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate = false);
        StRepSignal();

        virtual ~StRepSignal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void update() override;
    };


    class StSignal : public vsg::Inherit<AutoBlockSignal, StSignal>
    {
        Q_OBJECT
    public:
        StSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate = false);
        StSignal();

        virtual ~StSignal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void update() override;

        enum FwdHint : int
        {
            CLOSE_SIG,
            NO,
            RESTR40,
            RESTR60,
            RESTR80,
            PREP_RESTR80
        };

        void open(FwdHint hint);
        void close();

    protected:

        FwdHint _restrHint = CLOSE_SIG;
    };

    class EnterSignal : public vsg::Inherit<StSignal, EnterSignal>
    {
        Q_OBJECT
    public:
        EnterSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate = false);
        EnterSignal();

        virtual ~EnterSignal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void update() override;

    protected:
        LightAnimation *_y2anim;
        LightAnimation *_wanim;

        QSequentialAnimationGroup *_wloop;

    private:
        void initSigs(vsg::ref_ptr<vsg::Light> y2, vsg::ref_ptr<vsg::Light> w);
    };

    class ExitSignal : public vsg::Inherit<StSignal, ExitSignal>
    {
        Q_OBJECT
    public:
        ExitSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate = false);
        ExitSignal();

        virtual ~ExitSignal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void update() override;

    protected:

    };

}
#endif // SIGNAL_H
