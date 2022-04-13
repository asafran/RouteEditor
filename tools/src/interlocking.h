#ifndef INTERLOCKING_H
#define INTERLOCKING_H

#include <vsg/core/Inherit.h>
#include "trajectory.h"
#include "signal.h"
#include <QObject>

class RouteBeginModel;
class RouteEndModel;

namespace signalling
{
    class Route;
    class Command : public vsg::Inherit<vsg::Object, Command>
    {
    public:
        Command() : vsg::Inherit<vsg::Object, Command>() {}

        virtual void assemble() = 0;
        virtual void disassemble() = 0;

    };

    class JunctionCommand : public vsg::Inherit<Command, JunctionCommand>
    {
    public:
        void assemble() override;
        void disassemble() override;

        void read(vsg::Input &input) override;
        void write(vsg::Output &output) const override;

        vsg::ref_ptr<route::Junction> j;
        bool hint;
    };

    class SignalCommand : public vsg::Inherit<Command, SignalCommand>
    {
    public:
        void assemble() override;
        void disassemble() override;

        void read(vsg::Input &input) override;
        void write(vsg::Output &output) const override;

        vsg::ref_ptr<Signal> sig;
        State onHint;
        State offHint;
    };


    class RouteCommand : public vsg::Inherit<Command, RouteCommand>
    {
    public:
        void assemble() override;
        void disassemble() override;

        void read(vsg::Input &input) override;
        void write(vsg::Output &output) const override;

        vsg::ref_ptr<Route> rt;
    };

    class Route : public QObject, public vsg::Inherit<vsg::Object, Route>
    {
        Q_OBJECT
    public:
        Route();

        void read(vsg::Input &input) override;
        void write(vsg::Output &output) const override;

        static void* operator new(std::size_t count, void* ptr);
        static void* operator new(std::size_t count);
        static void operator delete(void* ptr);

        bool disassemble();
        bool assemble();
        bool onlyJcts();

        std::vector<vsg::ref_ptr<Command>> commands;
        std::vector<vsg::ref_ptr<route::Trajectory>> trajs;

    public slots:
        void passed(int ref);

    private:
        int _count = 0;
    };

    class Routes : public vsg::Inherit<vsg::Object, Routes>
    {
    public:
        void read(vsg::Input &input) override;
        void write(vsg::Output &output) const override;

        std::map<Signal*, vsg::ref_ptr<Route>> routes;
    };

    class Station : public vsg::Inherit<vsg::Object, Station>
    {
    public:
        void read(vsg::Input &input) override;
        void write(vsg::Output &output) const override;

        std::map<Signal*, vsg::ref_ptr<Routes>> rsignals;
        std::vector<Signal*> shunt;
    };
}

#endif // INTERLOCKING_H
