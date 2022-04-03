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
    class Stage;

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
        JunctionCommand(Junction *j, bool state);

        void assemble() override;
        void disassemble() override;

        void read(vsg::Input &input) override;
        void write(vsg::Output &output) const override;

        vsg::ref_ptr<Junction> _j;
        bool _hint;
    };

    class SignalCommand : public vsg::Inherit<Command, SignalCommand>
    {
    public:
        SignalCommand(StSignal *sig, StSignal::FwdHint hint);

        void assemble() override;
        void disassemble() override;

        void read(vsg::Input &input) override;
        void write(vsg::Output &output) const override;

        vsg::ref_ptr<StSignal> _sig;
        StSignal::FwdHint _hint;
    };

    class StageCommand : public vsg::Inherit<Command, StageCommand>
    {
    public:
        StageCommand(Stage *stg, bool dir);

        void assemble() override;
        void disassemble() override;

        void read(vsg::Input &input) override;
        void write(vsg::Output &output) const override;

        vsg::ref_ptr<Stage> _stg;
        bool _dir;
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

        std::vector<vsg::ref_ptr<Command>> commands;
        std::vector<vsg::ref_ptr<Trajectory>> trajs;

    public slots:
        void passed(int ref);

    private:
        int _count = 0;
    };

    class Routes : public vsg::Inherit<vsg::Object, Routes>
    {
    public:
        Routes();

        void read(vsg::Input &input) override;
        void write(vsg::Output &output) const override;

        std::map<Signal*, vsg::ref_ptr<Route>> routes;
    };

    class Stage : public vsg::Inherit<vsg::Object, Stage>
    {
    public:
        Stage();

        void read(vsg::Input &input) override;
        void write(vsg::Output &output) const override;

        std::vector<Signal*> fwdSignals;
        std::vector<Signal*> bwdSignals;
    };

    class Station : public vsg::Inherit<vsg::Object, Station>
    {
    public:
        Station();

        void read(vsg::Input &input) override;
        void write(vsg::Output &output) const override;

        std::map<Signal*, vsg::ref_ptr<Routes>> rsignals;
        std::map<Station*, vsg::ref_ptr<Stage>> stages;
    };
}

#endif // INTERLOCKING_H
