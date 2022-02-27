#ifndef INTERLOCKING_H
#define INTERLOCKING_H

#include <vsg/core/Inherit.h>
#include "trajectory.h"
#include "signal.h"
#include <QObject>

class RouteBeginModel;
class RouteEndModel;

namespace route
{
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
        JunctionCommand(vsg::ref_ptr<Junction> j, bool state);

        void assemble() override;
        void disassemble() override;

        void read(vsg::Input &input) override;
        void write(vsg::Output &output) const override;

    private:
        vsg::ref_ptr<Junction> _j;
        bool _hint;
    };

    class SignalCommand : public vsg::Inherit<Command, SignalCommand>
    {
    public:
        SignalCommand(vsg::ref_ptr<StSignal> sig, StSignal::FwdHint hint);

        void assemble() override;
        void disassemble() override;

        void read(vsg::Input &input) override;
        void write(vsg::Output &output) const override;

    private:
        vsg::ref_ptr<StSignal> _sig;
        StSignal::FwdHint _hint;
    };

    class Route : public QObject, public vsg::Inherit<vsg::Object, Route>
    {
        Q_OBJECT
    public:
        Route();

        void read(vsg::Input &input) override;
        void write(vsg::Output &output) const override;

        bool disassemble();
        bool assemble();
        bool onlyJcts();

    public slots:
        void passed(int ref);

    private:
        std::vector<Command> _commands; //other
        std::vector<JunctionCommand> _jcommands;
        int _count = 0;
        std::vector<vsg::ref_ptr<Trajectory>> _trajs;
    };

    class Routes : public vsg::Inherit<vsg::Object, Routes>
    {
    public:
        Routes();

        void read(vsg::Input &input) override;
        void write(vsg::Output &output) const override;

    private:
        std::map<Signal*, Route> _routes;

        friend class ::RouteEndModel;
    };

    class Station : public vsg::Inherit<vsg::Object, Station>
    {
    public:
        Station();

        void read(vsg::Input &input) override;
        void write(vsg::Output &output) const override;

    private:
        std::map<Signal*, Routes> _signals;

        friend class ::RouteBeginModel;
    };
}

#endif // INTERLOCKING_H
