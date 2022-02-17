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
    SplineTrajectory::SplineTrajectory(std::string name,
                                       vsg::ref_ptr<RailConnector> bwdPoint,
                                       vsg::ref_ptr<RailConnector> fwdPoint,
                                       vsg::ref_ptr<vsg::Builder> builder,
                                       std::string railPath,
                                       std::string fillPath,
                                       vsg::ref_ptr<vsg::Node> sleeper, double distance, double gaudge)
      : vsg::Inherit<Trajectory, SplineTrajectory>(name)
      , _builder(builder)
      , _track(vsg::MatrixTransform::create())
      , _railPath(railPath)
      , _fillPath(fillPath)
      , _sleeper(sleeper)
      , _sleepersDistance(distance)
      , _gaudge(gaudge)
      , _fwdPoint(fwdPoint)
      , _bwdPoint(bwdPoint)
    {

        fwdPoint->setBwd(this);
        bwdPoint->setFwd(this);

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
        _bwdPoint->setFwdNull(this);
        _fwdPoint->setBwdNull(this);
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
        _fwdPoint->setBwdNull(this);
        _fwdPoint = rc;
        _fwdPoint->setBwd(this);
    }

    void SplineTrajectory::setBwdPoint(RailConnector *rc)
    {
        _bwdPoint->setFwdNull(this);
        _bwdPoint = rc;
        _bwdPoint->setFwd(this);
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

    void SplineTrajectory::updateAttached()
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

    vsg::ref_ptr<RailConnector> SplineTrajectory::getBwdPoint() const
    {
        return _bwdPoint;
    }

    vsg::ref_ptr<RailConnector> SplineTrajectory::getFwdPoint() const
    {
        return _fwdPoint;
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

    Junction::Junction(std::string name,
                       vsg::ref_ptr<RailConnector> bwdPoint,
                       vsg::ref_ptr<RailConnector> fwdPoint,
                       vsg::ref_ptr<RailConnector> fwd2Point,
                       vsg::ref_ptr<vsg::AnimationPath> strait,
                       vsg::ref_ptr<vsg::AnimationPath> side,
                       vsg::ref_ptr<vsg::AnimationPath> switcherPath,
                       vsg::ref_ptr<vsg::Node> rails,
                       vsg::ref_ptr<vsg::MatrixTransform> switcher)
      : vsg::Inherit<Trajectory, Junction>(name)
      , _strait(strait)
      , _side(side)
      , _fwdPoint(fwdPoint)
      , _fwd2Point(fwd2Point)
      , _bwdPoint(bwdPoint)
    {
        Q_ASSERT(_side->locations.size() > 1);
        Q_ASSERT(_strait->locations.size() > 1);


        fwdPoint->setBwd(this);
        fwd2Point->setBwd(this);
        bwdPoint->setFwd(this);
    }

    Junction::Junction()
      : vsg::Inherit<Trajectory, Junction>()
    {
    }
    Junction::~Junction()
    {
        _bwdPoint->setFwd(nullptr);
        _fwdPoint->setBwd(nullptr);
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

        input.read("fwdPoint", _fwdPoint);
        input.read("fwd2Point", _fwd2Point);
        input.read("bwdPoint", _bwdPoint);
    }

    void Junction::write(vsg::Output& output) const
    {
        Group::write(output);

        output.write("straitTrajecory", _strait);
        output.write("sideTrajecory", _side);
        output.write("switcher", _switcher);

        output.write("fwdPoint", _fwdPoint);
        output.write("fwd2Point", _fwd2Point);
        output.write("bwdPoint", _bwdPoint);
    }

    void Junction::setPosition(const vsg::dvec3 &pos)
    {
        _position = pos;
        auto mat = transform(vsg::dmat4());
        _fwdPoint->localToWorld = mat;
        _fwd2Point->localToWorld = mat;
        _bwdPoint->localToWorld = mat;
    }

    void Junction::setRotation(const vsg::dquat &rot)
    {
        _quat = rot;
        auto mat = transform(vsg::dmat4());
        _fwdPoint->localToWorld = mat;
        _fwd2Point->localToWorld = mat;
        _bwdPoint->localToWorld = mat;
    }

    vsg::dvec3 Junction::getCoordinate(double x) const
    {
        auto &path = _state ? _side : _strait;
        return transform(vsg::dmat4()) * path->computeLocation(x).position;
    }

    double Junction::invert(const vsg::dvec3 vec) const
    {
        return 0.0;
    }

    vsg::dmat4 Junction::getMatrixAt(double x) const
    {
        auto &path = _state ? _side : _strait;
        return transform(path->computeMatrix(x));
    }

    SceneTrajectory::SceneTrajectory()
        : vsg::Inherit<vsg::Group, SceneTrajectory>()
    {
    }
    SceneTrajectory::SceneTrajectory(Trajectory *traj)
        : SceneTrajectory()
    {
        children.emplace_back(traj);
    }

    SceneTrajectory::~SceneTrajectory()
    {
        auto trajectory = children.front();
        Q_ASSERT(trajectory);

        std::string name;
        trajectory->getValue(app::NAME, name);

        auto trj = _topology->trajectories.find(name);
        Q_ASSERT(trj != _topology->trajectories.end());
        _topology->trajectories.erase(trj);
    }

    void SceneTrajectory::read(vsg::Input& input)
    {
        Node::read(input);

        std::string name;
        input.read("trajName", name);

        _topology = input.options->objectCache->get(app::TOPOLOGY).cast<Topology>();

        addChild(_topology->trajectories.at(name));

    }

    void SceneTrajectory::write(vsg::Output& output) const
    {
        Node::write(output);

        auto trajectory = children.front();
        Q_ASSERT(trajectory);

        std::string name;
        trajectory->getValue(app::NAME, name);
        output.write("trajName", name);

    }
}
