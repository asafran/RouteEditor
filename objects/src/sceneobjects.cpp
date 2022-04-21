#include "sceneobjects.h"
#include "LambdaVisitor.h"
#include <vsg/maths/quat.h>
#include <QDir>
#include <vsg/io/read.h>
#include <vsg/traversals/ComputeBounds.h>
#include <vsg/nodes/LOD.h>
#include "signal.h"
#include "topology.h"

namespace route
{
    SceneObject::SceneObject(vsg::ref_ptr<vsg::Node> box,
                             const vsg::dvec3& pos,
                             const vsg::dquat& w_quat,
                             const vsg::dmat4 &ltw)
        : vsg::Inherit<vsg::Transform, SceneObject>()
        , _position(pos)
        , _quat(0.0, 0.0, 0.0, 1.0)
        , _selected(false)
        , _world_quat(w_quat)
        , localToWorld(ltw)
    {
        _wireframe->addChild(box);
    }

    SceneObject::SceneObject(vsg::ref_ptr<vsg::Node> loaded,
                             vsg::ref_ptr<vsg::Node> box,
                             const vsg::dvec3 &pos,
                             const vsg::dquat& w_quat,
                             const vsg::dmat4 &ltw)
        : SceneObject(box, pos, w_quat, ltw)
    {
        addChild(loaded);
        recalculateWireframe();
    }

    SceneObject::SceneObject() : vsg::Inherit<vsg::Transform, SceneObject>() , _selected(false) {}

    SceneObject::~SceneObject() {}

    void SceneObject::read(vsg::Input& input)
    {
        Group::read(input);

        input.read("quat", _quat);
        input.read("world_quat", _world_quat);
        input.read("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
        input.read("ltw", localToWorld);
        input.read("coord", _position);

        _wireframe->children.emplace_back(const_cast<vsg::Node*>(input.options->getObject<vsg::Node>(app::WIREFRAME)));
        recalculateWireframe();
    }

    void SceneObject::write(vsg::Output& output) const
    {
        Group::write(output);

        output.write("quat", _quat);
        output.write("world_quat", _world_quat);
        output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
        output.write("ltw", localToWorld);
        output.write("coord", _position);
    }
    vsg::dquat SceneObject::getWorldRotation() const
    {
        return mult(_world_quat, _quat);
    }

    void SceneObject::recalculateWireframe()
    {
        vsg::ComputeBounds cb;
        t_traverse(*this, cb);

        vsg::dvec3 centre((cb.bounds.min + cb.bounds.max) * 0.5);

        auto delta = cb.bounds.max - cb.bounds.min;

        _wireframe->matrix = vsg::scale(delta) * vsg::translate(centre);
    }

    vsg::dmat4 SceneObject::transform(const vsg::dmat4& m) const
    {
        auto matrix = vsg::rotate(mult(_world_quat, _quat));
        matrix[3][0] = _position[0];
        matrix[3][1] = _position[1];
        matrix[3][2] = _position[2];

        return m * matrix;
    }

    SingleLoader::SingleLoader(vsg::ref_ptr<vsg::Node> loaded,
                               vsg::ref_ptr<vsg::Node> box,
                               const std::string &in_file,
                               const vsg::dvec3 &pos,
                               const vsg::dquat &in_quat,
                               const vsg::dmat4 &wtl)
        : vsg::Inherit<SceneObject, SingleLoader>(loaded, box, pos, in_quat, wtl)
        , file(in_file)
    {
    }
    SingleLoader::SingleLoader()
    {
    }
    SingleLoader::~SingleLoader() {}

    void SingleLoader::read(vsg::Input& input)
    {
        Node::read(input);

        input.read("quat", _quat);
        input.read("world_quat", _world_quat);
        input.read("filename", file);
        vsg::Paths searchPaths = vsg::getEnvPaths("RRS2_ROOT");
        vsg::Path filename = vsg::findFile(file, searchPaths);
        addChild(vsg::read_cast<vsg::Node>(filename));

        input.read("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
        input.read("ltw", localToWorld);
        input.read("coord", _position);
    }

    void SingleLoader::write(vsg::Output& output) const
    {
        Node::write(output);

        output.write("quat", _quat);
        output.write("world_quat", _world_quat);
        output.write("filename", file);
        output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
        output.write("ltw", localToWorld);

        //vsg::dvec3 pos(matrix[3][0], matrix[3][1], matrix[3][2]);

        output.write("coord", _position);
    }

    TerrainPoint::TerrainPoint(vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copy,
                               vsg::ref_ptr<vsg::BufferInfo> buffer,
                               const vsg::dmat4 &ltw,
                               vsg::ref_ptr<vsg::Node> compiled,
                               vsg::ref_ptr<vsg::Node> box,
                               vsg::stride_iterator<vsg::vec3> point)
        : vsg::Inherit<SceneObject, TerrainPoint>(compiled, box, ltw * vsg::dvec3(*point))
        , _worldToLocal(vsg::inverse(ltw))
        , _info(buffer)
        , _copyBufferCmd(copy)
        , _vertex(point)
    {
        auto norm = vsg::normalize(ltw * vsg::dvec3(*point));
        _world_quat = vsg::dquat(vsg::dvec3(0.0, 0.0, 1.0), norm);
    }
    TerrainPoint::~TerrainPoint() {}

    void TerrainPoint::setPosition(const vsg::dvec3& position)
    {
        _position = position;
        *_vertex = _worldToLocal * position;
        _copyBufferCmd->copy(_info->data, _info);
    }

    RailPoint::RailPoint(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, const vsg::dvec3 &pos)
        : vsg::Inherit<SceneObject, RailPoint>(loaded, box, pos)
    {
        auto norm = vsg::normalize(pos);
        _world_quat = vsg::dquat(vsg::dvec3(0.0, 0.0, 1.0), norm);
        //always in world coordinates
        //_quat = quat;
    }
    RailPoint::RailPoint()
        : vsg::Inherit<SceneObject, RailPoint>()
    {
    }
    RailPoint::~RailPoint() {}

    void RailPoint::read(vsg::Input &input)
    {
        SceneObject::read(input);

        input.read("tangent", _tangent);
        input.read("tilt", _tilt);
        input.read("catheight", _cheight);

        /*vsg::ref_ptr<Trajectory> trj;
        input.readObject("fstTraj", trj);
        trajectory = trj.get();*/
    }

    void RailPoint::write(vsg::Output &output) const
    {
        SceneObject::write(output);

        output.write("tangent", _tangent);
        output.write("tilt", _tilt);
        output.write("catheight", _cheight);

        //output.writeObject("fstTraj", trajectory);
    }

    void RailPoint::setPosition(const vsg::dvec3& position)
    {
        SceneObject::setPosition(position);
        recalculate();
    }
    void RailPoint::setRotation(const vsg::dquat &rotation)
    {
        SceneObject::setRotation(rotation);
        recalculate();
    }

    void RailPoint::recalculate()
    {
        if(trajectory) trajectory->recalculate();
    }

    vsg::dvec3 RailPoint::getTangent() const
    {
        return vsg::rotate(mult(_world_quat, _quat)) * vsg::dvec3(0.0, _tangent, 0.0);
    }

    vsg::dquat RailPoint::getTilt() const
    {
        return vsg::dquat(vsg::radians(_tilt), vsg::dvec3(0.0, 1.0, 0.0));
    }
/*
    void RailPoint::setInclination(double i)
    {
        auto angle = std::atan(i * 0.001);
        setRotation(mult(_quat, vsg::dquat(angle, vsg::dvec3(1.0, 0.0, 0.0))));
    }*/

    RailConnector::RailConnector(vsg::ref_ptr<vsg::Node> loaded,
                                 vsg::ref_ptr<vsg::Node> box,
                                 const vsg::dvec3 &pos)
        : QObject(nullptr)
        , vsg::Inherit<RailPoint, RailConnector>(loaded, box, pos) {}

    RailConnector::RailConnector()
        : vsg::Inherit<RailPoint, RailConnector>()
        , QObject(nullptr) {}

    RailConnector::~RailConnector() {}

    void RailConnector::read(vsg::Input &input)
    {
        RailPoint::read(input);

        input.read("fwdSignal", _fwdSignal);
        input.read("bwdSignal", _bwdSignal);
        input.read("fwdConnected", fwdConnected);
        input.read("bwdConnected", bwdConnected);

        input.read("reverser", _reverser);

        input.read("staticConnector", staticConnector);

        if(bwdConnected)
            connect(_bwdSignal, &signalling::Signal::sendState, this, &RailConnector::sendFwdState);
        if(fwdConnected)
            connect(_fwdSignal, &signalling::Signal::sendState, this, &RailConnector::sendBwdState);
        //vsg::ref_ptr<Trajectory> trj;
        //input.readObject("sndTraj", trj);
        //fwdTrajectory = trj.get();
    }

    void RailConnector::write(vsg::Output &output) const
    {
        RailPoint::write(output);

        output.write("fwdSignal", _fwdSignal);
        output.write("bwdSignal", _bwdSignal);
        output.write("fwdConnected", fwdConnected);
        output.write("bwdConnected", bwdConnected);

        output.write("reverser", _reverser);

        output.write("staticConnector", staticConnector);

        //output.writeObject("sndTraj", fwdTrajectory);
    }

    void RailConnector::setPosition(const vsg::dvec3 &position)
    {
        if(staticConnector)
            return;

        RailPoint::setPosition(position);
        if(_fwdSignal)
            _fwdSignal->localToWorld = getWorldTransform();
        if(_bwdSignal)
            _bwdSignal->localToWorld = getWorldTransform();
    }

    void RailConnector::setRotation(const vsg::dquat &rotation)
    {
        if(staticConnector)
            return;

        RailPoint::setRotation(rotation);
        if(_fwdSignal)
            _fwdSignal->localToWorld = getWorldTransform();
        if(_bwdSignal)
            _bwdSignal->localToWorld = getWorldTransform();
    }

    void RailConnector::recalculate()
    {
        RailPoint::recalculate();
        if(fwdTrajectory) fwdTrajectory->recalculate();
    }

    std::pair<Trajectory*, bool> RailConnector::getFwd(const Trajectory *caller) const
    {
        if(!_reverser)
            return std::make_pair(fwdTrajectory, false);
        bool reversed = caller == fwdTrajectory;
        auto trj = reversed ? trajectory : fwdTrajectory;
        return std::make_pair(trj, reversed);
    }

    std::pair<Trajectory*, bool> RailConnector::getBwd(const Trajectory *caller) const
    {
        if(!_reverser)
            return std::make_pair(trajectory, false);
        bool reversed = caller == trajectory;
        auto trj = reversed ? fwdTrajectory : trajectory;
        return std::make_pair(trj, reversed);
    }

    void RailConnector::setFwd(Trajectory *caller)
    {
        if(fwdTrajectory == nullptr)
            fwdTrajectory = caller;
        else if(trajectory == nullptr)
        {
            _reverser = true;
            trajectory = caller;
        }
    }

    void RailConnector::setBwd(Trajectory *caller)
    {
        if(trajectory == nullptr)
            trajectory = caller;
        else if(fwdTrajectory == nullptr)
        {
            _reverser = true;
            fwdTrajectory = caller;
        }
    }
    void RailConnector::setFwdNull(Trajectory *caller)
    {
        if(caller == fwdTrajectory)
        {
            if(_reverser)
            {
                fwdTrajectory = trajectory;
                trajectory = nullptr;
                fwdTrajectory->connectSignalling();
            }
            else
                fwdTrajectory = nullptr;
        }
        else if(caller == trajectory)
            trajectory = nullptr;

        _reverser = false;
    }

    void RailConnector::setBwdNull(Trajectory *caller)
    {
        if(caller == trajectory)
        {
            if(_reverser)
            {
                trajectory = fwdTrajectory;
                fwdTrajectory = nullptr;
                trajectory->connectSignalling();
            }
            else
                trajectory = nullptr;
        }
        else if(caller == fwdTrajectory)
            fwdTrajectory = nullptr;

        _reverser = false;
    }

    bool RailConnector::isFree() const
    {
        return trajectory == nullptr || fwdTrajectory == nullptr;
    }

    void RailConnector::setSignal(vsg::ref_ptr<signalling::Signal> signal, bool connect)
    {
        if(_fwdSignal)
            disconnect(_fwdSignal, nullptr, this, nullptr);
        _fwdSignal = signal;
        fwdConnected = connect;
        signal->localToWorld = getWorldTransform();
        if(_fwdSignal && connect)
            QObject::connect(_fwdSignal, &signalling::Signal::sendState, this, &RailConnector::sendBwdState);
    }

    void RailConnector::setReverseSignal(vsg::ref_ptr<signalling::Signal> signal, bool connect)
    {
        if(_bwdSignal)
            disconnect(_bwdSignal, nullptr, this, nullptr);
        _bwdSignal = signal;
        bwdConnected = connect;
        signal->localToWorld = getWorldTransform();
        if(_bwdSignal && connect)
            QObject::connect(_bwdSignal, &signalling::Signal::sendState, this, &RailConnector::sendFwdState);
    }

    void RailConnector::traverse(vsg::Visitor &visitor)
    {
        Transform::traverse(visitor);
        if(_fwdSignal)
            _fwdSignal->accept(visitor);
        if(_bwdSignal)
            _bwdSignal->accept(visitor);
    }

    void RailConnector::traverse(vsg::ConstVisitor &visitor) const
    {
        Transform::traverse(visitor);
        if(_fwdSignal)
            _fwdSignal->accept(visitor);
        if(_bwdSignal)
            _bwdSignal->accept(visitor);
    }

    void RailConnector::traverse(vsg::RecordTraversal &visitor) const
    {
        SceneObject::traverse(visitor);
        if(_fwdSignal)
            _fwdSignal->accept(visitor);
        if(_bwdSignal)
            _bwdSignal->accept(visitor);
    }

    void RailConnector::receiveBwdDirState(signalling::State state)
    {
        if(!_fwdSignal)
            emit sendBwdState(state);
        else
        {
            _fwdSignal->setFwdState(state);
            if(!fwdConnected)
                emit sendBwdState(state);
        }

    }

    void RailConnector::receiveFwdDirState(signalling::State state)
    {
        if(!_bwdSignal)
            emit sendFwdState(state);
        else
        {
            _bwdSignal->setFwdState(state);
            if(!bwdConnected)
                emit sendFwdState(state);
        }
    }

    void RailConnector::receiveFwdDirRef(int c)
    {
        if(!_bwdSignal)
            emit sendFwdRef(c);
        else
        {
            _bwdSignal->Ref(c);
            if(!bwdConnected)
                emit sendFwdRef(c);
        }
    }

    void RailConnector::receiveBwdDirRef(int c)
    {
        if(!_fwdSignal)
            emit sendBwdRef(c);
        else
        {
            _fwdSignal->Ref(c);
            if(!fwdConnected)
                emit sendBwdRef(c);
        }
    }
/*
    void RailConnector::receiveFwdDirUnref()
    {
        if(!_bwdSignal)
            emit sendFwdUnref();
        else
            _bwdSignal->Unref();
    }

    void RailConnector::receiveBwdDirUnref()
    {
        if(!_fwdSignal)
            emit sendBwdUnref();
        else
            _fwdSignal->Unref();
    }
*/
    vsg::ref_ptr<signalling::Signal> RailConnector::bwdSignal() const
    {
        return _bwdSignal;
    }

    vsg::ref_ptr<signalling::Signal> RailConnector::fwdSignal() const
    {
        return _fwdSignal;
    }

    void* RailConnector::operator new(std::size_t count, void* ptr)
    {
        return ::operator new(count, ptr);
    }

    void* RailConnector::operator new(std::size_t count)
    {
        return vsg::allocate(count, vsg::ALLOCATOR_AFFINITY_OBJECTS);
    }

    void RailConnector::operator delete(void* ptr)
    {
        vsg::deallocate(ptr);
    }

    SwitchConnector::SwitchConnector(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, const vsg::dvec3 &pos)
        : vsg::Inherit<RailConnector, SwitchConnector>(loaded, box, pos)
    {
        _world_quat = {0.0, 0.0, 0.0, 1.0};

        _fwdSignal = signalling::Signal::create(vsg::Node::create(), vsg::Node::create());
        _bwdSignal = signalling::Signal::create(vsg::Node::create(), vsg::Node::create());
        _sideCounter = signalling::Signal::create(vsg::Node::create(), vsg::Node::create());

        staticConnector = true;

        _state = false;
    }

    SwitchConnector::SwitchConnector()
    {
    }

    SwitchConnector::~SwitchConnector()
    {
    }

    void SwitchConnector::read(vsg::Input &input)
    {
        RailConnector::read(input);

        input.read("default", _state);
        input.read("counter", _sideCounter);

        /*vsg::ref_ptr<Trajectory> trj;
        input.readObject("sideTraj", trj);
        sideTrajectory = trj.get();*/
    }

    void SwitchConnector::write(vsg::Output &output) const
    {
        RailConnector::write(output);

        output.write("default", _state);
        output.write("counter", _sideCounter);

        //output.writeObject("sideTraj", sideTrajectory);
    }

    void SwitchConnector::setFwd(Trajectory *caller)
    {
        if(fwdTrajectory == nullptr)
            fwdTrajectory = caller;
        else if(sideTrajectory == nullptr)
            sideTrajectory = caller;
        else if(trajectory == nullptr)
        {
            _reverser = true;
            trajectory = caller;
        }
    }
/*
    std::pair<Trajectory *, bool> SwitchConnector::getFwd(const Trajectory *caller) const
    {
        return std::make_pair(_state ? sideTrajectory : fwdTrajectory, false);
    }

    std::pair<Trajectory *, bool> SwitchConnector::getBwd(const Trajectory *caller) const
    {
        if(!_reverser)
            return std::make_pair(trajectory, false);
        bool reversed = caller == trajectory;
        auto trj = reversed ? (_state ? sideTrajectory : fwdTrajectory) : trajectory;
        return std::make_pair(trj, reversed);
    }
*/
    void SwitchConnector::switchState(bool state)
    {
        if(state == _state)
            return;
        if(state)
        {
            emit sendFwdSideRef(_bwdSignal->_vcount);
            emit sendFwdRef(-_bwdSignal->_vcount);

            emit sendBwdState(_sideCounter->_front);
            emit sendFwdSideState(_bwdSignal->_front);

            emit sendBwdRef(_sideCounter->_vcount);
            emit sendBwdRef(-_fwdSignal->_vcount);

            emit sendFwdSideRef(-1);
            emit sendFwdRef(1);

            _state = state;
        }
        else
        {
            emit sendFwdRef(_bwdSignal->_vcount);
            emit sendFwdSideRef(-_bwdSignal->_vcount);

            emit sendBwdState(_fwdSignal->_front);
            emit sendFwdState(_bwdSignal->_front);

            emit sendBwdRef(_fwdSignal->_vcount);
            emit sendBwdRef(-_sideCounter->_vcount);

            emit sendFwdSideRef(1);
            emit sendFwdRef(-1);

            _state = state;
        }
        auto tmp = sideTrajectory;
        sideTrajectory = fwdTrajectory;
        fwdTrajectory = tmp;
    }

    void SwitchConnector::receiveBwdSideDirState(signalling::State state)
    {
        _sideCounter->setFwdState(state);
        if(_state)
            emit sendBwdState(state);
    }

    void SwitchConnector::receiveBwdDirState(signalling::State state)
    {
        _fwdSignal->setFwdState(state);
        if(!_state)
            emit sendBwdState(state);
    }

    void SwitchConnector::receiveFwdDirState(signalling::State state)
    {
        _bwdSignal->setFwdState(state);
        if(_state)
            emit sendFwdSideState(state);
        else
            emit sendFwdState(state);
    }

    void SwitchConnector::receiveFwdDirRef(int c)
    {
        _bwdSignal->Ref(c);
        if(_state)
            emit sendFwdSideRef(c);
    }

    void SwitchConnector::receiveBwdDirRef(int c)
    {
        _fwdSignal->Ref(c);
        if(!_state)
            emit sendBwdRef(c);
    }

    void SwitchConnector::receiveBwdSideDirRef(int c)
    {
        _sideCounter->Ref(c);
        if(_state)
            emit sendBwdRef(c);
    }

}
