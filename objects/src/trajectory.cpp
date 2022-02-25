#include    "trajectory.h"

#include    <QFile>
#include    <QDir>
#include    <QTextStream>
#include    <execution>
#include    <vsg/io/read.h>
#include    "topology.h"
#include    "sceneobjects.h"
#include    "LambdaVisitor.h"
#include    <vsg/nodes/VertexIndexDraw.h>
#include    <vsg/io/ObjectCache.h>



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

        attach();
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
                transform.matrix = getMatrixAt(coord);
        };
        LambdaVisitor<decltype (computeTransform), vsg::MatrixTransform> ct(computeTransform);
        Trajectory::accept(ct);
    }

    SplineTrajectory::SplineTrajectory(std::string name,
                                       vsg::ref_ptr<RailConnector> bwdPoint,
                                       vsg::ref_ptr<RailConnector> fwdPoint,
                                       vsg::ref_ptr<vsg::Builder> builder,
                                       std::string railPath,
                                       std::string fillPath,
                                       vsg::ref_ptr<vsg::Node> sleeper, double distance, double gaudge)
      : vsg::Inherit<Trajectory, SplineTrajectory>(name, bwdPoint, fwdPoint)
      , _builder(builder)
      , _track(vsg::MatrixTransform::create())
      , _railPath(railPath)
      , _fillPath(fillPath)
      , _sleeper(sleeper)
      , _sleepersDistance(distance)
      , _gaudge(gaudge)
    {

        reloadData();
        //subgraphRequiresLocalFrustum = false;

        SplineTrajectory::recalculate();
    }

    SplineTrajectory::SplineTrajectory()
      : vsg::Inherit<Trajectory, SplineTrajectory>()
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
        Group::read(input);

        input.read("sleepersDistance", _sleepersDistance);
        input.read("gaudge", _gaudge);
        input.read("autoPositioned", _autoPositioned);
        input.read("points", _points);
        input.read("railPath", _railPath);
        input.read("fillPath", _fillPath);
        input.read("sleeper", _sleeper);

        input.read("fwdPoint", _fwdPoint);
        input.read("bwdPoint", _bwdPoint);

        input.read("track", _track);

        reloadData();

        updateSpline();
    }

    void SplineTrajectory::write(vsg::Output& output) const
    {
        Group::write(output);

        output.write("sleepersDistance", _sleepersDistance);
        output.write("gaudge", _gaudge);
        output.write("autoPositioned", _autoPositioned);
        output.write("points", _points);
        output.write("railPath", _railPath);
        output.write("fillPath", _fillPath);
        output.write("sleeper", _sleeper);

        output.write("fwdPoint", _fwdPoint);
        output.write("bwdPoint", _bwdPoint);

        output.write("track", _track);
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

    vsg::dmat4 SplineTrajectory::getMatrixAt(double x) const
    {
        double T = ArcLength::solveLength(*_railSpline, 0.0, x);
        auto pt = _railSpline->getTangent(T);
        return InterpolatedPTM(std::move(pt), mixTilt(T)).calculated;
    }

    void SplineTrajectory::updateSpline()
    {
        std::vector<vsg::dvec3> points;
        std::vector<vsg::dvec3> tangents;

        points.push_back(_fwdPoint->getPosition());
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

        points.push_back(_bwdPoint->getPosition());
        if(_bwdPoint->getBwd(this).second)
            tangents.push_back(-_bwdPoint->getTangent());
        else
            tangents.push_back(_bwdPoint->getTangent());

        _railSpline.reset(new InterpolationSpline(points, tangents));
    }

    void SplineTrajectory::recalculate()
    {
        updateSpline();

        auto n = std::ceil(_railSpline->totalLength() / _sleepersDistance);

        auto partitionBoundaries = ArcLength::partitionN(*_railSpline, static_cast<size_t>(n));

        auto front = _fwdPoint->getPosition();
        _track->matrix = vsg::translate(front);

        std::vector<InterpolatedPTM> derivatives(partitionBoundaries.size());
        std::transform(std::execution::par_unseq, partitionBoundaries.begin(), partitionBoundaries.end(), derivatives.begin(),
                       [this, front](double T)
        {
            return std::move(InterpolatedPTM(_railSpline->getTangent(T), mixTilt(T), front));
        });

        _track->children.clear();
        size_t index = 0;
        for (auto &ptcm : derivatives)
        {
            auto transform = vsg::MatrixTransform::create(ptcm.calculated);
            transform->addChild(_sleeper);
            _track->addChild(transform);
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

    vsg::ref_ptr<vsg::VertexIndexDraw> SplineTrajectory::createGeometry(const vsg::vec3 &offset,
                                                                        const std::vector<InterpolatedPTM> &derivatives,
                                                                        const std::vector<VertexData> &geometry) const
    {
        std::vector<std::pair<std::vector<vsg::vec3>, size_t>> vertexGroups(derivatives.size());

        std::transform(std::execution::par_unseq, derivatives.begin(), derivatives.end(), vertexGroups.begin(),
        [geometry, offset](const InterpolatedPTM &ptcm)
        {
            auto fmat = static_cast<vsg::mat4>(ptcm.calculated);
            std::vector<vsg::vec3> out;

            for(const auto &vec : geometry)
                out.push_back(fmat * (vec.verticle + offset));

            return std::make_pair(std::move(out), ptcm.index);
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
            for(auto i = 0; i < vertexGroup.first.size(); ++i)
            {
                Q_ASSERT(vertIt != vertArray->end() && texIt != texArray->end() && normIt != normalArray->end());
                *vertIt = vertexGroup.first.at(i);
                *texIt = vsg::vec2(vertexGroup.second, geometry.at(i).uv);
                *normIt = geometry.at(i).normal;
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

        auto vid = vsg::VertexIndexDraw::create();

        vid->assignArrays(vsg::DataList{vertArray, normalArray, texArray});

        vid->assignIndices(ind);
        vid->indexCount = static_cast<uint32_t>(ind->size());
        vid->instanceCount = 1;

        return vid;
    }

    void SplineTrajectory::assignRails(const std::vector<InterpolatedPTM> &derivatives)
    {
        auto left = createGeometry(vsg::vec3(_gaudge / 2.0, 0.0, 0.0), derivatives, _rail);
        auto right = createGeometry(vsg::vec3(-_gaudge / 2.0, 0.0, 0.0), derivatives, _rail);
        auto fill = createGeometry(vsg::vec3(), derivatives, _fill);


        auto rstateGroup = createStateGroup(_railTexture);
        rstateGroup->addChild(left);
        rstateGroup->addChild(right);

        auto fstateGroup = createStateGroup(_fillTexture);

        fstateGroup->addChild(fill);

        _builder->compileTraversal->compile(rstateGroup);
        _builder->compileTraversal->compile(fstateGroup);

        _track->addChild(rstateGroup);
        _track->addChild(fstateGroup);

    }

    std::pair<std::vector<SplineTrajectory::VertexData>, vsg::ref_ptr<vsg::Data>> SplineTrajectory::loadData(std::string path)
    {
        QFileInfo fi(path.c_str());

        tinyobj::ObjReaderConfig reader_config;
        reader_config.mtl_search_path = fi.absolutePath().toStdString(); // Path to material files

        tinyobj::ObjReader reader;
        std::vector<VertexData> vd;

        if (!reader.ParseFromFile(path, reader_config))
        {
            std::make_pair(vd, nullptr);
        }

        auto& materials = reader.GetMaterials();

        auto texture = vsg::read_cast<vsg::Data>(fi.absolutePath().toStdString() + "/" + materials.front().diffuse_texname, _builder->options);

        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();

        auto it = attrib.vertices.begin();
        auto end = attrib.vertices.end() - (attrib.vertices.size()/2);
        while (it < end)
        {
            vd.push_back(VertexData(vsg::vec3(*it, *(it + 2), *(it + 1))));
            it += 3;
        }

        for(const auto &index : shapes.front().mesh.indices)
        {
            if(index.vertex_index < vd.size())
            {
                auto& v = vd[index.vertex_index];
                v.uv = attrib.texcoords.at((index.texcoord_index * 2) + 1);
                auto idx = index.normal_index * 3;
                v.normal.x = attrib.normals.at(idx);
                v.normal.y = attrib.normals.at(idx + 1);
                v.normal.z = attrib.normals.at(idx + 2);
            }
        }
        return std::make_pair(vd, texture);
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
            rp->setRotation(InterpolatedPTM(_railSpline->getTangent(T)).rot);
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

    void SplineTrajectory::reloadData()
    {
        auto rail = loadData(_railPath);
        auto fill = loadData(_fillPath);

        _rail = rail.first;
        _railTexture = rail.second;

        _fill = fill.first;
        _fillTexture = fill.second;
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
    }

    PointsTrajectory::PointsTrajectory() {}

    PointsTrajectory::~PointsTrajectory()
    {

    }

    void PointsTrajectory::read(vsg::Input &input)
    {

    }

    void PointsTrajectory::write(vsg::Output &output) const
    {

    }

    vsg::dvec3 PointsTrajectory::getCoordinate(double x) const
    {
        return localToWorld * _path->computeLocation(x).position;
    }

    double PointsTrajectory::invert(const vsg::dvec3 vec) const
    {
        return 0.0;
    }

    vsg::dmat4 PointsTrajectory::getMatrixAt(double x) const
    {
        return localToWorld * _path->computeMatrix(x);
    }

    vsg::dmat4 PointsTrajectory::getLocalMatrixAt(double x) const
    {
        return _path->computeMatrix(x);
    }

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

        auto fwdStrPoint = StaticConnector::create(mrk, box, strait->locations.crbegin()->second.position);
        auto fwdSiPoint = StaticConnector::create(mrk, box, side->locations.crbegin()->second.position);

        _strait = PointsTrajectory::create(name + "_str", _switcherPoint.cast<RailConnector>(), fwdStrPoint, strait);
        _side = PointsTrajectory::create(name + "_side", _switcherPoint, fwdSiPoint, side);
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
    }
}
