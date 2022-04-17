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
#include <QQueue>

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
        Signal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, int pause = 1000, int duration = 1000);
        Signal();

        virtual ~Signal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void setState(State hint);

        virtual void setFwdState(signalling::State front);
        virtual void Ref(int c);

        static void* operator new(std::size_t count, void* ptr);
        static void* operator new(std::size_t count);
        static void operator delete(void* ptr);

        std::string station;

    signals:
        void sendCode(signalling::Code code);
        void sendState(signalling::State state);

    protected:
        void clear();

        virtual QAbstractAnimation *getAnim(State state, State front);

        State _front = Off;

        State _state = Off;

        QSequentialAnimationGroup _group = new QSequentialAnimationGroup();

        int _vcount = 0;

        float _intensity = 0.0f;

        int _pause = 1000;
        int _duration = 1000;

        friend class route::SwitchConnector;
    };

    class ShSignal : public vsg::Inherit<Signal, ShSignal>
    {
        Q_OBJECT
    public:
        ShSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, int pause, int duration);
        ShSignal();

        virtual ~ShSignal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void setFwdState(signalling::State front) override;
        void Ref(int c) override;

    protected:
        QAbstractAnimation* getAnim(State state, State front) override;

        vsg::ref_ptr<vsg::Light> _s;
        vsg::ref_ptr<vsg::Light> _w;
    };

    class Sh2Signal : public vsg::Inherit<ShSignal, Sh2Signal>
    {
        Q_OBJECT
    public:
        Sh2Signal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, int pause, int duration);
        Sh2Signal();

        virtual ~Sh2Signal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

    protected:
        QAbstractAnimation* getAnim(State state, State front) override;

        vsg::ref_ptr<vsg::Light> _w1;
    };

    class AutoBlockSignal : public vsg::Inherit<Signal, AutoBlockSignal>
    {
        Q_OBJECT
    public:
        AutoBlockSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, int pause, int duration, bool fstate = false);
        AutoBlockSignal();

        virtual ~AutoBlockSignal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void Ref(int c) override;

    protected:
        QAbstractAnimation* getAnim(State state, State front) override;

        vsg::ref_ptr<vsg::Light> _r;
        vsg::ref_ptr<vsg::Light> _y;
        vsg::ref_ptr<vsg::Light> _g;

        bool _fstate = false;
    };

    class StRepSignal : public vsg::Inherit<AutoBlockSignal, StRepSignal>
    {
        Q_OBJECT
    public:
        StRepSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box,
                    int pause,
                    int duration,
                    int pauseOn,
                    int pauseOff,
                    bool fstate = false);
        StRepSignal();

        virtual ~StRepSignal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

    protected:
        QAbstractAnimation* getAnim(State state, State front) override;

        int _pauseOn = 2000;
        int _pauseOff = 1000;

    };
/*
    class RouteRepSignal : public vsg::Inherit<Signal, RouteRepSignal>
    {
        Q_OBJECT
    public:
        RouteRepSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box);
        RouteRepSignal();

        virtual ~RouteRepSignal();

        void read(vsg::Input& input) override;

    protected:
        QAbstractAnimation* getAnim(State state, State front) override;

        QSequentialAnimationGroup *_gloop;
        QSequentialAnimationGroup *_yloop;

    private:
        void initSigs();
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

        State processState() const override;
        QAbstractAnimation* offState() override;
        QAbstractAnimation* applyState(State state) override;
    };*/

    class RouteSignal : public vsg::Inherit<StRepSignal, RouteSignal>
    {
        Q_OBJECT
    public:
        RouteSignal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, int pause, int duration,
                    int pauseOn,
                    int pauseOff,
                    bool fstate = false);
        RouteSignal();

        virtual ~RouteSignal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

    protected:
        QAbstractAnimation* getAnim(State state, State front) override;

        vsg::ref_ptr<vsg::Light> _y1;
        vsg::ref_ptr<vsg::Light> _w;
    };

    class RouteV2Signal : public vsg::Inherit<RouteSignal, RouteV2Signal>
    {
        Q_OBJECT
    public:
        RouteV2Signal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, int pause, int duration,
                      int pauseOn,
                      int pauseOff,
                      bool fstate = false);
        RouteV2Signal();

        virtual ~RouteV2Signal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

    protected:
        QAbstractAnimation* getAnim(State state, State front) override;

        vsg::ref_ptr<vsg::Light> _gline;
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

EVSG_type_name(signalling::AutoBlockSignal);
EVSG_type_name(signalling::StRepSignal);
EVSG_type_name(signalling::ShSignal);
EVSG_type_name(signalling::Sh2Signal);
EVSG_type_name(signalling::RouteSignal);
EVSG_type_name(signalling::RouteV2Signal);
EVSG_type_name(signalling::Signal);

#endif // SIGNAL_H
