#ifndef SIGNAL_H
#define SIGNAL_H

#include "sceneobjects.h"
#include "trajectory.h"
#include <vsg/nodes/Switch.h>
#include "Constants.h"

namespace route
{
    class Signal : public QObject, public vsg::Inherit<SceneObject, Signal>
    {
    public:
        Signal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box);
        Signal();

        virtual ~Signal();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

    public slots:
        virtual void setFwdState(route::State state) = 0;
        void ref();
        void unref();

    signals:
        void sendCode(route::Code code);

    protected:
        vsg::ref_ptr<Trajectory> _code;

        int vcount = 0;
    };

    class AutoBlockSignal3 : public vsg::Inherit<Signal, AutoBlockSignal3>
    {
    public:
        AutoBlockSignal3(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box);
        AutoBlockSignal3();

        virtual ~AutoBlockSignal3();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void setFwd(AutoBlockSignal3 *_front);

        void traverse(vsg::Visitor& visitor) override { Transform::traverse(visitor); _signals.at(_state)->accept(visitor); }
        void traverse(vsg::ConstVisitor& visitor) const override { Transform::traverse(visitor); _signals.at(_state)->accept(visitor); }
        void traverse(vsg::RecordTraversal& visitor) const override { SceneObject::traverse(visitor); _signals.at(_state)->accept(visitor); }

    public slots:
        void update() override;

    protected:

        std::array<vsg::ref_ptr<vsg::Node>,3> _signals;

        State _state = CLOSED;

        bool _fstate = false;

        AutoBlockSignal3 *_front = nullptr;
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
