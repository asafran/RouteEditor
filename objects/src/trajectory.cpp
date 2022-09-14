#include    "trajectory.h"

#include    <QFile>
#include    <QDir>
#include    <QTextStream>
//#include    <execution>
#include    <vsg/io/read.h>
#include    "topology.h"
#include    "sceneobjects.h"
#include    "LambdaVisitor.h"
#include    <vsg/nodes/VertexIndexDraw.h>
#include    <vsg/io/ObjectCache.h>
#include    <vsg/viewer/Viewer.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "../tiny_obj_loader.h"

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

namespace route
{
    Trajectory::Trajectory(std::string name,
                           vsg::ref_ptr<RailConnector> bwdPoint,
                           vsg::ref_ptr<RailConnector> fwdPoint)
        : QObject(nullptr), vsg::Inherit<vsg::Group, Trajectory>()
        , _fwdPoint(fwdPoint)
        , _bwdPoint(bwdPoint)
    {
        setValue(app::NAME, name);
    }

    Trajectory::Trajectory() : QObject(nullptr) {}

    Trajectory::~Trajectory() {}

    void Trajectory::detatch()
    {
        disconnect(_fwdPoint, nullptr, _bwdPoint, nullptr);
        disconnect(_bwdPoint, nullptr, _fwdPoint, nullptr);
        disconnect(this, nullptr, _bwdPoint, nullptr);
        disconnect(this, nullptr, _fwdPoint, nullptr);

        _bwdPoint->setFwdNull(this);
        _fwdPoint->setBwdNull(this);
    }

    void Trajectory::attach()
    {
        _fwdPoint->setBwd(this);
        _bwdPoint->setFwd(this);

        connectSignalling();
    }

    void Trajectory::connectSignalling()
    {
        auto front = _fwdPoint->getFwd(this);
        auto back =  _bwdPoint->getBwd(this);
        bool frontReversed = front.second;
        bool backReversed = back.second;

        disconnect(_fwdPoint, frontReversed ? &RailConnector::sendFwdState : &RailConnector::sendBwdState, nullptr, nullptr);
        disconnect(_fwdPoint, frontReversed ? &RailConnector::sendFwdRef : &RailConnector::sendBwdRef, nullptr, nullptr);
        //disconnect(_fwdPoint, frontReversed ? &RailConnector::sendFwdUnref : &RailConnector::sendBwdUnref, nullptr, nullptr);

        disconnect(_bwdPoint, backReversed ? &RailConnector::sendBwdState : &RailConnector::sendFwdState, nullptr, nullptr);
        disconnect(_bwdPoint, backReversed ? &RailConnector::sendBwdRef : &RailConnector::sendFwdRef, nullptr, nullptr);
        //disconnect(_bwdPoint, backReversed ? &RailConnector::sendBwdUnref : &RailConnector::sendFwdUnref, nullptr, nullptr);

        connect(this, &Trajectory::sendRef, _fwdPoint, frontReversed ? &RailConnector::receiveBwdDirRef : &RailConnector::receiveFwdDirRef);
        connect(this, &Trajectory::sendRef, _bwdPoint, backReversed ? &RailConnector::receiveFwdDirRef : &RailConnector::receiveBwdDirRef);
        //connect(this, &Trajectory::sendUnref, _fwdPoint, frontReversed ? &RailConnector::receiveBwdDirUnref : &RailConnector::receiveFwdDirUnref);
        //connect(this, &Trajectory::sendUnref, _bwdPoint, backReversed ? &RailConnector::receiveFwdDirUnref : &RailConnector::receiveBwdDirUnref);

        connect(_fwdPoint, frontReversed ? &RailConnector::sendFwdState : &RailConnector::sendBwdState,
                _bwdPoint, backReversed ? &RailConnector::receiveFwdDirState : &RailConnector::receiveBwdDirState);
        connect(_fwdPoint, frontReversed ? &RailConnector::sendFwdRef : &RailConnector::sendBwdRef,
                _bwdPoint, backReversed ? &RailConnector::receiveFwdDirRef : &RailConnector::receiveBwdDirRef);
        //connect(_fwdPoint, frontReversed ? &RailConnector::sendFwdUnref : &RailConnector::sendBwdUnref,
                //_bwdPoint, backReversed ? &RailConnector::receiveFwdDirUnref : &RailConnector::receiveBwdDirUnref);

        connect(_bwdPoint, backReversed ? &RailConnector::sendBwdState : &RailConnector::sendFwdState,
                _fwdPoint, frontReversed ? &RailConnector::receiveBwdDirState : &RailConnector::receiveFwdDirState);
        connect(_bwdPoint, backReversed ? &RailConnector::sendBwdRef : &RailConnector::sendFwdRef,
                _fwdPoint, frontReversed ? &RailConnector::receiveBwdDirRef : &RailConnector::receiveFwdDirRef);
        //connect(_bwdPoint, backReversed ? &RailConnector::sendBwdUnref : &RailConnector::sendFwdUnref,
                //_fwdPoint, frontReversed ? &RailConnector::receiveBwdDirUnref : &RailConnector::receiveFwdDirUnref);
    }

    void Trajectory::updateAttached()
    {
        auto computeTransform = [this](vsg::MatrixTransform& transform)
        {
            double coord = 0.0;
            if(transform.getValue(app::PROP, coord))
                transform.matrix = getMatrixAt(coord).first;
        };
        LambdaVisitor<decltype (computeTransform), vsg::MatrixTransform> ct(computeTransform);
        Trajectory::accept(ct);
    }

    void* Trajectory::operator new(std::size_t count, void* ptr)
    {
        return ::operator new(count, ptr);
    }

    void* Trajectory::operator new(std::size_t count)
    {
        return vsg::allocate(count, vsg::ALLOCATOR_AFFINITY_OBJECTS);
    }

    void Trajectory::operator delete(void* ptr)
    {
        vsg::deallocate(ptr);
    }

    StraitTrajectory::StraitTrajectory(std::string name,
                                       vsg::ref_ptr<RailConnector> bwdPoint,
                                       vsg::ref_ptr<RailConnector> fwdPoint,
                                       vsg::ref_ptr<vsg::Options> options,
                                       std::string railPath,
                                       std::string fillPath,
                                       vsg::ref_ptr<vsg::Node> sleeper, double distance, double gaudge)
        : vsg::Inherit<Trajectory, StraitTrajectory>(name, bwdPoint, fwdPoint)
        , _track(vsg::MatrixTransform::create())
        , _railL(vsg::VertexIndexDraw::create())
        , _railR(vsg::VertexIndexDraw::create())
        , _fill(vsg::VertexIndexDraw::create())
        , _railPath(railPath)
        , _fillPath(fillPath)
        , _sleeper(sleeper)
        , _sleepersDistance(distance)
        , _gaudge(gaudge)
    {
        reloadData(options);
        _viewer = const_cast<vsg::Viewer*>(options->getObject<vsg::Viewer>(app::VIEWER));
    }

    StraitTrajectory::StraitTrajectory()
      : vsg::Inherit<Trajectory, StraitTrajectory>()
      , _track(vsg::MatrixTransform::create())
      , _railL(vsg::VertexIndexDraw::create())
      , _railR(vsg::VertexIndexDraw::create())
      , _fill(vsg::VertexIndexDraw::create())
    {
    }

    StraitTrajectory::~StraitTrajectory()
    {

    }

    vsg::dvec3 StraitTrajectory::getCoordinate(double x) const
    {
        auto T = x / _lenght;
        return vsg::mix(_fwdPoint->getWorldPosition(), _bwdPoint->getWorldPosition(), T);
    }

    double StraitTrajectory::invert(const vsg::dvec3 vec) const
    {
        return vsg::length(_fwdPoint->getWorldPosition() - vec);
    }

    std::pair<vsg::dmat4, double> StraitTrajectory::getMatrixAt(double x) const
    {
        auto T = x / _lenght;
        auto pos = vsg::mix(_fwdPoint->getWorldPosition(), _bwdPoint->getWorldPosition(), T);
        auto quat = _fwdPoint->getRotation();
        return std::pair(vsg::translate(pos) * vsg::rotate(_fwdPoint->getWorldRotation()), toElevation(quat));
    }

    void StraitTrajectory::read(vsg::Input &input)
    {
        Group::read(input);

        input.read("sleepersDistance", _sleepersDistance);
        input.read("gaudge", _gaudge);

        input.readObjects("autoPositioned", _autoPositioned);

        input.read("railPath", _railPath);
        input.read("fillPath", _fillPath);
        input.read("sleeper", _sleeper);

        input.read("fwdPoint", _fwdPoint);
        input.read("bwdPoint", _bwdPoint);

        bool frontRev = false;
        bool bwdRev = false;

        input.read("fwdPointRev", frontRev);
        input.read("bwdPointRev", bwdRev);

        _viewer = const_cast<vsg::Viewer*>(input.options->getObject<vsg::Viewer>(app::VIEWER));

        if(frontRev)
            _fwdPoint->fwdTrajectory = this;
        else
            _fwdPoint->trajectory = this;

        if(bwdRev)
            _bwdPoint->trajectory = this;
        else
            _bwdPoint->fwdTrajectory = this;

        reloadData(input.options);

        recalculate();

        connectSignalling();
    }

    void StraitTrajectory::write(vsg::Output &output) const
    {
        Group::write(output);

        output.write("sleepersDistance", _sleepersDistance);
        output.write("gaudge", _gaudge);

        output.writeObjects("autoPositioned", _autoPositioned);

        output.write("railPath", _railPath);
        output.write("fillPath", _fillPath);
        output.write("sleeper", _sleeper);

        output.write("fwdPoint", _fwdPoint);
        output.write("bwdPoint", _bwdPoint);

        bool frontRev = _fwdPoint->getFwd(this).second;
        bool bwdRev = _bwdPoint->getBwd(this).second;

        output.write("fwdPointRev", frontRev);
        output.write("bwdPointRev", bwdRev);
    }

    void StraitTrajectory::recalculate()
    {
        auto delta = _bwdPoint->getWorldPosition() - _fwdPoint->getWorldPosition();

        bool frontRev = _fwdPoint->getFwd(this).second;
        bool bwdRev = _bwdPoint->getBwd(this).second;

        _lenght = vsg::length(delta);

        auto w_quat = _fwdPoint->getWorldQuat();

        auto tangent = vsg::normalize(vsg::inverse(vsg::rotate(w_quat)) * delta);

        double zangle = -std::atan2(tangent.x, tangent.y);
        double xangle = std::atan2(tangent.z, vsg::length(vsg::dvec2(tangent.x, tangent.y)));

        auto rot = mult(vsg::dquat(zangle, vsg::dvec3(0.0, 0.0, 1.0)), vsg::dquat(xangle, vsg::dvec3(1.0, 0.0, 0.0)));

        if(_fwdPoint->staticConnector)
        {
            _bwdPoint->_quat = (frontRev || bwdRev) ? -_fwdPoint->getRotation() : _fwdPoint->getRotation();
            _bwdPoint->setWorldPositionNoUpdate((vsg::normalize(_fwdPoint->getTangent()) * (frontRev ? -_lenght : _lenght)) + _fwdPoint->getPosition());
            if(!bwdRev &&_bwdPoint->trajectory) _bwdPoint->trajectory->recalculate();
            else if(bwdRev && _bwdPoint->fwdTrajectory) _bwdPoint->fwdTrajectory->recalculate();
        }
        else if(_bwdPoint->staticConnector)
        {
            _fwdPoint->_quat = (frontRev || bwdRev) ? -_bwdPoint->getRotation() : _bwdPoint->getRotation();
            _fwdPoint->setWorldPositionNoUpdate((vsg::normalize(_bwdPoint->getTangent()) * (bwdRev ? _lenght : -_lenght)) + _bwdPoint->getPosition());
            if(!frontRev && _fwdPoint->fwdTrajectory) _fwdPoint->fwdTrajectory->recalculate();
            else if(frontRev && _fwdPoint->trajectory) _fwdPoint->trajectory->recalculate();
        }
        else
        {
            _fwdPoint->_quat = frontRev ? mult(rot, vsg::dquat(vsg::PI, {0.0, 0.0, 1.0})) : rot;
            _bwdPoint->_quat = bwdRev ? mult(rot, vsg::dquat(vsg::PI, {0.0, 0.0, 1.0})) : rot;

            if(!frontRev && _fwdPoint->fwdTrajectory) _fwdPoint->fwdTrajectory->recalculate();
            else if(frontRev && _fwdPoint->trajectory) _fwdPoint->trajectory->recalculate();

            if(!bwdRev &&_bwdPoint->trajectory) _bwdPoint->trajectory->recalculate();
            else if(bwdRev && _bwdPoint->fwdTrajectory) _bwdPoint->fwdTrajectory->recalculate();
        }

        auto fwdPos = _fwdPoint->getWorldPosition();
        auto bwdPos = _bwdPoint->getWorldPosition();

        _track->matrix = vsg::translate(fwdPos);

        auto group = vsg::Group::create();

        _track->children.pop_back();
        _track->addChild(group);

        std::vector<InterpolatedPTM> derivatives;

        auto fwdTan = frontRev ? -_fwdPoint->getTangent() : _fwdPoint->getTangent();
        auto bwdTan = bwdRev ? -_bwdPoint->getTangent() : _bwdPoint->getTangent();

        derivatives.emplace_back(InterpolatedPTM(std::move(InterpolationSpline::InterpolatedPT{fwdPos, fwdTan}), vsg::dquat(0.0, 0.0, 0.0, 1.0), fwdPos));

        derivatives.emplace_back(InterpolatedPTM(std::move(InterpolationSpline::InterpolatedPT{bwdPos, bwdTan}), vsg::dquat(0.0, 0.0, 0.0, 1.0), fwdPos));

        derivatives[1].index = 1;

        auto n = std::ceil(_lenght / _sleepersDistance);

        if(n > 0.1)
        {
            auto deltaT = 1 / n;
            for (double T = 0.0; T <= 1.0; T += deltaT)
            {
                auto transform = vsg::MatrixTransform::create(vsg::translate(vsg::mix(fwdPos, bwdPos, T) - fwdPos) * derivatives.front().calculated);
                transform->addChild(_sleeper);
                group->addChild(transform);
                derivatives[1].index++;
            }
        }

        assignRails(derivatives);

        updateAttached();
    }

    void StraitTrajectory::reloadData(vsg::ref_ptr<const vsg::Options> options)
    {
        _track->children.clear();

        auto rail = loadData(_railPath, options);
        auto fill = loadData(_fillPath, options);

        _railGeo = rail.first;
        rail.second->addChild(_railL);
        rail.second->addChild(_railR);

        _track->addChild(rail.second);

        _fillGeo = fill.first;
        fill.second->addChild(_fill);

        _track->addChild(fill.second);

        auto group = vsg::Group::create();
        _track->addChild(group);
    }

    void StraitTrajectory::assignRails(const std::vector<InterpolatedPTM> &derivatives)
    {
        assignGeometry(vsg::vec3(_gaudge / 2.0, 0.0, 0.0), derivatives, _railGeo, _railL);
        assignGeometry(vsg::vec3(-_gaudge / 2.0, 0.0, 0.0), derivatives, _railGeo, _railR);
        assignGeometry(vsg::vec3(), derivatives, _fillGeo, _fill);

        if(_viewer)
        {
            auto result = _viewer->compileManager->compile(_track);
            vsg::updateViewer(*_viewer, result);
        }
    }

    void StraitTrajectory::assignGeometry(const vsg::vec3 &offset, const std::vector<InterpolatedPTM> &derivatives, const std::vector<VertexData> &geometry, vsg::ref_ptr<vsg::VertexIndexDraw> vid) const
    {
        std::vector<std::vector<VertexData>> vertexGroups(derivatives.size());

        std::transform(derivatives.begin(), derivatives.end(), vertexGroups.begin(),
        [geometry, offset](const InterpolatedPTM &ptcm)
        {
            auto fmat = static_cast<vsg::mat4>(ptcm.calculated);
            auto frtmat = static_cast<vsg::mat4>(ptcm.wrotation);
            std::vector<VertexData> out;

            for(const auto &vec : geometry)
            {
                VertexData vd(fmat * (vec.verticle + offset));
                vd.normal = frtmat * vec.normal;
                vd.uv = vsg::vec2(ptcm.index, vec.uv.y);
                out.push_back(vd);
            }

            return out;
        });

        auto vsize = vertexGroups.size() * geometry.size();

        auto vertArray = vsg::vec3Array::create(vsize);
        auto texArray = vsg::vec2Array::create(vsize);
        auto normalArray = vsg::vec3Array::create(vsize);

        auto vertIt = vertArray->begin();
        auto texIt = texArray->begin();
        auto normIt = normalArray->begin();

        for(const auto &vertexGroup : vertexGroups)
        {
            //Q_ASSERT(vertexGroup.first.size() == geometry.size());
            for(const auto &vertex : vertexGroup)
            {
                Q_ASSERT(vertIt != vertArray->end() && texIt != texArray->end() && normIt != normalArray->end());
                *vertIt = vertex.verticle;
                *texIt = vertex.uv;
                *normIt = vertex.normal;
                vertIt++;
                texIt++;
                normIt++;
            }
        }

        std::vector<uint16_t> indices;
        const auto next = static_cast<uint16_t>(geometry.size());
        uint16_t max = vsize - next - 1;

        for (uint16_t ind = 0; ind < max; ++ind)
        {
            if((ind + 1) % next == 0)
                continue;
            indices.push_back(ind);
            indices.push_back(ind + next);
            indices.push_back(ind + next + 1);
            indices.push_back(ind + next + 1);
            indices.push_back(ind + 1);
            indices.push_back(ind);
        }

        //auto colorArray = vsg::vec4Array::create(vsize);
        //std::fill(colorArray->begin(), colorArray->end(), vsg::vec4(1.0f, 0.0f, 1.0f, 1.0f));

        auto ind = vsg::ushortArray::create(indices.size());
        std::copy(indices.begin(), indices.end(), ind->begin());

        auto colors = vsg::vec4Array::create({{1.0f, 1.0f, 1.0f, 1.0f}});

        vsg::DataList arrays;
        arrays.push_back(vertArray);
        arrays.push_back(normalArray);
        arrays.push_back(texArray);
        arrays.push_back(colors);
        //if (positions) arrays.push_back(positions);
        vid->assignArrays(arrays);

        vid->assignIndices(ind);
        vid->indexCount = static_cast<uint32_t>(ind->size());
        vid->instanceCount = 1;
    }



    SplineTrajectory::SplineTrajectory(std::string name,
                                       vsg::ref_ptr<RailConnector> bwdPoint,
                                       vsg::ref_ptr<RailConnector> fwdPoint,
                                       vsg::ref_ptr<vsg::Options> options,
                                       std::string railPath,
                                       std::string fillPath,
                                       vsg::ref_ptr<vsg::Node> sleeper, double distance, double gaudge)
      : vsg::Inherit<StraitTrajectory, SplineTrajectory>(name, bwdPoint, fwdPoint, options, railPath, fillPath, sleeper, distance, gaudge)

    {

        //reloadData(builder->options);
        //subgraphRequiresLocalFrustum = false;

        //SplineTrajectory::recalculate();
    }

    SplineTrajectory::SplineTrajectory()
      : vsg::Inherit<StraitTrajectory, SplineTrajectory>()
    {
    }
    SplineTrajectory::~SplineTrajectory()
    {
    }
    //------------------------------------------------------------------------------
    //
    //------------------------------------------------------------------------------

    void SplineTrajectory::read(vsg::Input& input)
    {
        StraitTrajectory::read(input);

        input.readObjects("points", _points);

        for(auto& point : _points)
            point->trajectory = this;

        recalculate();
    }

    void SplineTrajectory::write(vsg::Output& output) const
    {
        StraitTrajectory::write(output);

        output.writeObjects("points", _points);
    }

    vsg::dvec3 SplineTrajectory::getCoordinate(double x) const
    {
        double T = ArcLength::solveLength(*_railSpline, 0.0, x);
        return _railSpline->getPosition(T);
    }

    double SplineTrajectory::invert(const vsg::dvec3 vec) const
    {
        SplineInverter<vsg::dvec3, double> inverter(*_railSpline);
        auto T = inverter.findClosestT(vec);
        return _railSpline->arcLength(0, T);
    }

    std::pair<vsg::dmat4, double> SplineTrajectory::getMatrixAt(double x) const
    {
        double T = ArcLength::solveLength(*_railSpline, 0.0, x);
        auto pt = _railSpline->getTangent(T);
        InterpolatedPTM ptm(std::move(pt), mixTilt(T));
        return std::make_pair(ptm.calculated, ptm.tangent.z / vsg::length(vsg::dvec2(ptm.tangent.x, ptm.tangent.y)) * 1000);
    }

    void SplineTrajectory::updateSpline()
    {
        std::vector<vsg::dvec3> points;
        std::vector<vsg::dvec3> tangents;

        points.push_back(_fwdPoint->getWorldPosition());
        if(_fwdPoint->getFwd(this).second)
            tangents.push_back(-_fwdPoint->getTangent());
        else
            tangents.push_back(_fwdPoint->getTangent());

        std::transform(_points.begin(), _points.end(), std::back_insert_iterator(points),
                       [](const vsg::ref_ptr<RailPoint> sp)
        {
            return sp->getPosition();
        });
        std::transform(_points.begin(), _points.end(), std::back_insert_iterator(tangents),
                       [](const vsg::ref_ptr<RailPoint> sp)
        {
            return sp->getTangent();
        });

        points.push_back(_bwdPoint->getWorldPosition());
        if(_bwdPoint->getBwd(this).second)
            tangents.push_back(-_bwdPoint->getTangent());
        else
            tangents.push_back(_bwdPoint->getTangent());

        _railSpline.reset(new InterpolationSpline(points, tangents));
    }

    void SplineTrajectory::recalculate()
    {
        updateSpline();

        _lenght = _railSpline->totalLength();

        auto n = std::ceil(_lenght / _sleepersDistance);

        auto partitionBoundaries = ArcLength::partitionN(*_railSpline, static_cast<size_t>(n));

        auto front = _fwdPoint->getPosition();
        _track->matrix = vsg::translate(front);

        std::vector<InterpolatedPTM> derivatives(partitionBoundaries.size());
        std::transform(partitionBoundaries.begin(), partitionBoundaries.end(), derivatives.begin(),
                       [this, front](double T)
        {
            return std::move(InterpolatedPTM(_railSpline->getTangent(T), mixTilt(T), front));
        });

        auto group = vsg::Group::create();

        _track->children.pop_back();
        _track->addChild(group);

        size_t index = 0;
        for (auto &ptcm : derivatives)
        {
            auto transform = vsg::MatrixTransform::create(ptcm.calculated);
            transform->addChild(_sleeper);
            group->addChild(transform);
            ptcm.index = index;
            index++;
        }

        //auto last = std::unique(std::execution::par, derivatives.begin(), derivatives.end()); //vectorization is not supported
        //derivatives.erase(last, derivatives.end());

        assignRails(derivatives);

        updateAttached();
    }

    void SplineTrajectory::setFwdPoint(RailConnector *rc)
    {
        disconnect(_fwdPoint, nullptr, _bwdPoint, nullptr);
        disconnect(_bwdPoint, nullptr, _fwdPoint, nullptr);

        _fwdPoint->setBwdNull(this);
        _fwdPoint = rc;
        _fwdPoint->setBwd(this);

        connectSignalling();
    }

    void SplineTrajectory::setBwdPoint(RailConnector *rc)
    {
        disconnect(_fwdPoint, nullptr, _bwdPoint, nullptr);
        disconnect(_bwdPoint, nullptr, _fwdPoint, nullptr);

        _bwdPoint->setFwdNull(this);
        _bwdPoint = rc;
        _bwdPoint->setFwd(this);

        connectSignalling();
    }

    vsg::ref_ptr<RailPoint> SplineTrajectory::findFloorPoint(double t) const
    {
        auto index = static_cast<int>(t) - 1;
        if(index < 0)
            return _fwdPoint;
        else if(index >= _points.size())
            return _bwdPoint;
        else
            return _points.at(index);
    }

    vsg::dquat SplineTrajectory::mixTilt(double T) const
    {
        double i;
        double fr = std::modf(T, &i);
        return vsg::mix(findFloorPoint(i)->getTilt(), findFloorPoint(i + 1.0)->getTilt(), fr);
    }

    void SplineTrajectory::add(vsg::ref_ptr<RailPoint> rp, bool autoRotate)
    {
        SplineInverter<vsg::dvec3, double> inverter(*_railSpline);
        double T = inverter.findClosestT(rp->getPosition());

        if(T >= _railSpline->getMaxT())
            _points.push_back(rp);
        else
        {
            auto index = static_cast<size_t>(std::trunc(T));
            auto it = _points.begin() + index;
            Q_ASSERT(it <= _points.end());
            _points.insert(it, rp);
        }
        rp->trajectory = this;

        if(autoRotate)
            rp->setRotation(InterpolatedPTM(_railSpline->getTangent(T)).rotation);
        else
            recalculate(); //called on setRotation
    }

    void SplineTrajectory::remove(size_t index)
    {
        auto it = _points.begin() + index;
        Q_ASSERT(it < _points.end());
        _points.erase(it);

        recalculate();
    }

    void SplineTrajectory::remove(vsg::ref_ptr<RailPoint> rp)
    {
        auto it = std::find(_points.begin(), _points.end(), rp);
        Q_ASSERT(it != _points.end());
        _points.erase(it);

        recalculate();
    }

    PointsTrajectory::PointsTrajectory(std::string name,
                                       vsg::ref_ptr<RailConnector> bwdPoint,
                                       vsg::ref_ptr<RailConnector> fwdPoint,
                                       vsg::ref_ptr<vsg::AnimationPath> path)
        : vsg::Inherit<Trajectory, PointsTrajectory>(name, bwdPoint, fwdPoint)
        , _path(path)
    {

    }

    PointsTrajectory::PointsTrajectory(std::string name,
                                       vsg::ref_ptr<SwitchConnector> bwdPoint,
                                       vsg::ref_ptr<RailConnector> fwdPoint,
                                       vsg::ref_ptr<vsg::AnimationPath> path)
        : vsg::Inherit<Trajectory, PointsTrajectory>()
        , _path(path)
    {
        setValue(app::NAME, name);

         _bwdPoint = bwdPoint;
         _fwdPoint = fwdPoint;

        fwdPoint->setBwd(this);
        bwdPoint->setFwd(this);

        connect(this, &Trajectory::sendRef, fwdPoint, &RailConnector::receiveFwdDirRef);
        connect(this, &Trajectory::sendRef, bwdPoint, &SwitchConnector::receiveBwdSideDirRef);

        connect(fwdPoint, &RailConnector::sendBwdState, bwdPoint, &SwitchConnector::receiveBwdSideDirState);
        connect(fwdPoint, &RailConnector::sendBwdRef, bwdPoint, &SwitchConnector::receiveBwdSideDirRef);

        connect(bwdPoint, &SwitchConnector::sendFwdSideState, fwdPoint, &RailConnector::receiveFwdDirState);
        connect(bwdPoint, &SwitchConnector::sendFwdSideRef, fwdPoint, &RailConnector::receiveFwdDirRef);

        fwdPoint->receiveFwdDirRef(1);
    }

    PointsTrajectory::PointsTrajectory() {}

    PointsTrajectory::~PointsTrajectory()
    {

    }

    void PointsTrajectory::read(vsg::Input &input)
    {
        Group::read(input);

        input.read("path", _path);

        input.read("fwdPoint", _fwdPoint);
        input.read("bwdPoint", _bwdPoint);

        _fwdPoint->trajectory = this;
        _bwdPoint->setFwd(this);
    }

    void PointsTrajectory::write(vsg::Output &output) const
    {
        Group::write(output);

        output.write("path", _path);

        output.write("fwdPoint", _fwdPoint);
        output.write("bwdPoint", _bwdPoint);
    }

    vsg::dvec3 PointsTrajectory::getCoordinate(double x) const
    {
        return localToWorld * _path->computeLocation(x).position;
    }

    double PointsTrajectory::invert(const vsg::dvec3 vec) const
    {
        return 0.0;
    }

    std::pair<vsg::dmat4, double> PointsTrajectory::getMatrixAt(double x) const
    {
        return std::make_pair(localToWorld * _path->computeMatrix(x), elevation);
    }
/*
    vsg::dmat4 PointsTrajectory::getLocalMatrixAt(double x) const
    {
        return _path->computeMatrix(x);
    }
*/
    Junction::Junction(std::string name,
                       vsg::ref_ptr<vsg::AnimationPath> strait,
                       vsg::ref_ptr<vsg::AnimationPath> side,
                       vsg::ref_ptr<vsg::AnimationPath> switcherPath,
                       vsg::ref_ptr<vsg::Node> mrk,
                       vsg::ref_ptr<vsg::Node> box,
                       vsg::ref_ptr<vsg::Node> rails,
                       vsg::ref_ptr<vsg::MatrixTransform> switcher)
      : vsg::Inherit<SceneObject, Junction>(box, rails)
    {
        Q_ASSERT(side->locations.size() > 1);
        Q_ASSERT(strait->locations.size() > 1);

        _switcherPoint = SwitchConnector::create(mrk, box, strait->locations.cbegin()->second.position);

        auto fwdStrPoint = RailConnector::create(mrk, box, strait->locations.crbegin()->second.position);
        auto fwdSiPoint = RailConnector::create(mrk, box, side->locations.crbegin()->second.position);

        fwdSiPoint->staticConnector = true;
        fwdStrPoint->staticConnector = true;

        _strait = PointsTrajectory::create(name + "_str", _switcherPoint.cast<RailConnector>(), fwdStrPoint, strait); //as traj
        _side = PointsTrajectory::create(name + "_side", _switcherPoint, fwdSiPoint, side); //as side
    }

    Junction::Junction()
      : vsg::Inherit<SceneObject, Junction>()
    {
    }
    Junction::~Junction()
    {
    }
    //------------------------------------------------------------------------------
    //
    //------------------------------------------------------------------------------

    void Junction::read(vsg::Input& input)
    {
        Group::read(input);

        input.read("straitTrajecory", _strait);
        input.read("sideTrajecory", _side);
        input.read("switcher", _switcher);

        setRotation(_quat);

    }

    void Junction::write(vsg::Output& output) const
    {
        Group::write(output);

        output.write("straitTrajecory", _strait);
        output.write("sideTrajecory", _side);
        output.write("switcher", _switcher);

    }

    void Junction::setPosition(const vsg::dvec3 &pos)
    {
        _position = pos;
        auto mat = transform(vsg::dmat4());
        _strait->localToWorld = mat;
        _side->localToWorld = mat;
    }

    void Junction::setRotation(const vsg::dquat &rot)
    {
        _quat = rot;
        auto mat = transform(vsg::dmat4());
        _strait->localToWorld = mat;
        _side->localToWorld = mat;
        auto elevation = toElevation(rot);
        _strait->elevation = elevation;
        _side->elevation = elevation;
    }

    void Junction::setState(bool state)
    {
        _switcherPoint->switchState(state);
    }
}
