#ifndef SIGNAL_H
#define SIGNAL_H

#include "sceneobjects.h"
#include "trajectory.h"
#include <vsg/nodes/Switch.h>
#include <vsg/nodes/Light.h>
#include "Constants.h"

namespace route
{
    class Signal : public QObject, public vsg::Inherit<SceneObject, Signal>
    {
        Q_OBJECT
    public:
        Signal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box);
        Signal();

        virtual ~Signal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        virtual void update() { }
        void setFwdState(route::State state) { _front = state; update(); }
        void Ref(int c);

    signals:
        void sendCode(route::Code code);
        void sendState(route::State state);

    protected:
        vsg::ref_ptr<Trajectory> _code;

        State _front = CLOSED;

        int _vcount = 0;

        float _intensity = 3.0f;

        friend class route::SwitchConnector;
    };

    class AutoBlockSignal3 : public vsg::Inherit<Signal, AutoBlockSignal3>
    {
        Q_OBJECT
    public:
        AutoBlockSignal3(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, bool fstate = false);
        AutoBlockSignal3();

        virtual ~AutoBlockSignal3();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void update() override;

        enum Sig
        {
            R,
            Y,
            G,
            GY
        };

    protected:

        std::array<vsg::ref_ptr<vsg::Light>,3> _signals;

        State _state = OPENED;

        Sig _signal = G;

        bool _fstate = false;
        bool _repeater = false;
    };
/*
    class AutoBlockSignal4 : public vsg::Inherit<AutoBlockSignal3, AutoBlockSignal4>
    {
    public:
        AutoBlockSignal4(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box);
        AutoBlockSignal4();

        template<class N, class V>
        static void t_traverse(N& node, V& visitor)
        {
            if(node._state == GY)
            {
                node._signals.at(G)->accept(visitor);
                node._signals.at(Y)->accept(visitor);
            }
            else
                node._signals.at(node._state)->accept(visitor);
        }

        void traverse(vsg::Visitor& visitor) override { Transform::traverse(visitor); t_traverse(*this, visitor); }
        void traverse(vsg::ConstVisitor& visitor) const override { Transform::traverse(visitor); t_traverse(*this, visitor); }
        void traverse(vsg::RecordTraversal& visitor) const override { SceneObject::traverse(visitor); t_traverse(*this, visitor); }

    public slots:
        void update() override;

    private:
        enum State
        {
            G,
            Y,
            R,
            GY
        };

        State _state = R;

        AutoBlockSignal4 *_front;
    };*/
}
#endif // SIGNAL_H
