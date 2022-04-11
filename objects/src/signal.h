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

        void set(Hint hint);

        virtual State processState() const;
        virtual QAbstractAnimation* offState();
        virtual QAbstractAnimation* applyState(State state);

        virtual void setFwdState(signalling::State state) { _front = state; update(); }
        virtual void Ref(int c);

        std::string station;

    signals:
        void sendCode(signalling::Code code);
        void sendState(signalling::State state);

    protected:
        State _front = V0;

        State _state;

        Hint _hint;

        int _vcount = 0;

        float _intensity = 3.0f;

        friend class route::SwitchConnector;
    };

    class ShSignal : public vsg::Inherit<Signal, ShSignal>
    {
        Q_OBJECT
    public:
        ShSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box);
        ShSignal();

        virtual ~ShSignal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        State processState() const override;
        QAbstractAnimation* offState() override;
        QAbstractAnimation* applyState(State state) override;

        void setFwdState(signalling::State state) override;
        void Ref(int c) override;

    protected:

        LightAnimation *_sanim;
        LightAnimation *_wanim;

    private:
        void initSigs(vsg::ref_ptr<vsg::Light> s, vsg::ref_ptr<vsg::Light> w);
    };

    class Sh2Signal : public vsg::Inherit<ShSignal, Sh2Signal>
    {
        Q_OBJECT
    public:
        Sh2Signal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box);
        Sh2Signal();

        virtual ~Sh2Signal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        State processState() const override;
        QAbstractAnimation* offState() override;
        QAbstractAnimation* applyState(State state) override;

    protected:

        LightAnimation *_w1anim;

    private:
        void initSigs(vsg::ref_ptr<vsg::Light> w1);
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
        QAbstractAnimation* offState() override;
        QAbstractAnimation* applyState(State state) override;

    protected:

        LightAnimation *_ranim;
        LightAnimation *_yanim;
        LightAnimation *_ganim;

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

        State processState() const override;
        QAbstractAnimation* offState() override;
        QAbstractAnimation* applyState(State state) override;

    protected:
        QSequentialAnimationGroup *_gloop;
        QSequentialAnimationGroup *_yloop;

    private:
        void initSigs();
    };

/*
    class StSignal : public vsg::Inherit<AutoBlockSignal, StSignal>
    {
        Q_OBJECT
    public:
        StSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate = false);
        StSignal();

        virtual ~StSignal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        State processState() const override;
        QAbstractAnimation* offState() override;
        QAbstractAnimation* applyState(State state) override;
    };*/

    class EnterSignal : public vsg::Inherit<AutoBlockSignal, EnterSignal>
    {
        Q_OBJECT
    public:
        EnterSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate = false);
        EnterSignal();

        virtual ~EnterSignal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

    protected:
        LightAnimation *_y2anim;
        LightAnimation *_wanim;

        QSequentialAnimationGroup *_wloop;

    private:
        void initSigs(vsg::ref_ptr<vsg::Light> y2, vsg::ref_ptr<vsg::Light> w);
    };

    class ExitSignal : public vsg::Inherit<AutoBlockSignal, ExitSignal>
    {
        Q_OBJECT
    public:
        ExitSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate = false);
        ExitSignal();

        virtual ~ExitSignal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

    protected:

    };

}
#endif // SIGNAL_H
