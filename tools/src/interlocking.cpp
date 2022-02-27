#include "interlocking.h"

namespace route
{
    JunctionCommand::JunctionCommand(vsg::ref_ptr<Junction> j, bool state)
        : vsg::Inherit<Command, JunctionCommand>()
        , _j(j)
        , _hint(state)
    {
    }

    void JunctionCommand::assemble()
    {
        _j->setState(_hint);
    }

    void JunctionCommand::disassemble()
    {
        _j->setState(false);
    }
    void JunctionCommand::read(vsg::Input &input)
    {

    }

    void JunctionCommand::write(vsg::Output &output) const
    {
    }

    SignalCommand::SignalCommand(vsg::ref_ptr<StSignal> sig, StSignal::FwdHint hint)
        : vsg::Inherit<Command, SignalCommand>()
        , _sig(sig)
        , _hint(hint)
    {

    }

    void SignalCommand::assemble()
    {
        _sig->open(_hint);
    }

    void SignalCommand::disassemble()
    {
        _sig->close();
    }

    void SignalCommand::read(vsg::Input &input)
    {

    }

    void SignalCommand::write(vsg::Output &output) const
    {

    }

    Route::Route() : QObject(nullptr)
      , vsg::Inherit<vsg::Object, Route>()
    {

    }

    void Route::read(vsg::Input &input)
    {

    }

    void Route::write(vsg::Output &output) const
    {

    }

    void Route::passed(int ref)
    {
        _count += ref;
        if(_count == 0)
            disassemble();
    }

    bool Route::disassemble()
    {
        for(const auto& trj : _trajs)
            if(trj->isBusy())
                return false;
        for(auto& cmd : _commands) cmd.disassemble();
        for(auto& cmd : _jcommands) cmd.disassemble();
        return true;
    }
    bool Route::assemble()
    {
        for(const auto& trj : _trajs)
            if(trj->isBusy())
                return false;
        for(auto& cmd : _commands) cmd.assemble();
        for(auto& cmd : _jcommands) cmd.assemble();
        return true;
    }

    bool Route::onlyJcts()
    {
        for(const auto& trj : _trajs)
            if(trj->isBusy())
                return false;
        for(auto& cmd : _jcommands) cmd.assemble();
        return true;
    }

    Routes::Routes() : vsg::Inherit<vsg::Object, Routes>()
    {

    }

    void Routes::read(vsg::Input &input)
    {

    }

    void Routes::write(vsg::Output &output) const
    {

    }

    Station::Station() : vsg::Inherit<vsg::Object, Station>()
    {

    }

    void Station::read(vsg::Input &input)
    {

    }

    void Station::write(vsg::Output &output) const
    {

    }
}
