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

        void setState(State hint);

        virtual void setFwdState(signalling::State front);
        virtual void Ref(int c);

        std::string station;

    signals:
        void sendCode(signalling::Code code);
        void sendState(signalling::State state);

    protected:
        virtual QAbstractAnimation* getAnim(State state, State front);

        State _front = V0;

        State _state = Off;

        //Hint _hint = OffH;

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

        void setFwdState(signalling::State front) override;
        void Ref(int c) override;

    protected:
        QAbstractAnimation* getAnim(State state, State front) override;

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

    protected:
        QAbstractAnimation* getAnim(State state, State front) override;

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

        void Ref(int c) override;

    protected:
        QAbstractAnimation* getAnim(State state, State front) override;

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

    protected:
        QAbstractAnimation* getAnim(State state, State front) override;

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

    class RouteSignal : public vsg::Inherit<StRepSignal, RouteSignal>
    {
        Q_OBJECT
    public:
        RouteSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate = false);
        RouteSignal();

        virtual ~RouteSignal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

    protected:
        QAbstractAnimation* getAnim(State state, State front) override;

        LightAnimation *_y1anim;
        LightAnimation *_wanim;

        QSequentialAnimationGroup *_wloop;

    private:
        void initSigs(vsg::ref_ptr<vsg::Light> y1, vsg::ref_ptr<vsg::Light> w);
    };

    class RouteV2Signal : public vsg::Inherit<RouteSignal, RouteV2Signal>
    {
        Q_OBJECT
    public:
        RouteV2Signal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate = false);
        RouteV2Signal();

        virtual ~RouteV2Signal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

    protected:
        QAbstractAnimation* getAnim(State state, State front) override;

        LightAnimation *_glineanim;

    private:
        void initSigs(vsg::ref_ptr<vsg::Light> line);
    };
/*
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
        QAbstractAnimation* getAnim(State state, State front) override;

    };
*/
}
#endif // SIGNAL_H
