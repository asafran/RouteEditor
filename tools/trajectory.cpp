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
#include    <vsg/nodes/StateGroup.h>
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
                                       //vsg::ref_ptr<Compiler> compiler,
                                       tinyobj::attrib_t rail, vsg::ref_ptr<vsg::Data> texture,
                                       vsg::ref_ptr<vsg::Node> sleeper, double distance, double gaudge)
      : vsg::Inherit<Trajectory, SplineTrajectory>(name)
      , _builder(builder)
      //, _compiler(compiler)
      , _texture(texture)
      , _sleeper(sleeper)
      , _sleepersDistance(distance)
      , _gaudge(gaudge)
      , _fwdPoint(fwdPoint)
      , _bwdPoint(bwdPoint)
    {
        auto it = rail.vertices.begin();
        auto end = rail.vertices.end() - (rail.vertices.size()/2);
        while (it < end)
        {
            _geometry.push_back(vsg::vec3(*it, *(it + 1), *(it + 2)));
            it += 3;
        }

        fwdPoint->setBwd(this);
        bwdPoint->setFwd(this);

        it = rail.texcoords.begin();
        end = rail.texcoords.end() - (rail.texcoords.size()/2);
        while (it < end)
            _uv1.push_back(vsg::vec2(*it++, *it++));

        end = rail.texcoords.end();
        while (it < end)
            _uv2.push_back(vsg::vec2(*it++, *it++));

        SplineTrajectory::recalculate();
    }

    SplineTrajectory::SplineTrajectory()
      : vsg::Inherit<Trajectory, SplineTrajectory>()
    {
    }
    SplineTrajectory::~SplineTrajectory()
    {
        _bwdPoint->setFwd(nullptr);
        _fwdPoint->setBwd(nullptr);
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
        input.read("geometry", _geometry);
        input.read("uv1", _uv1);
        input.read("uv2", _uv2);
        input.read("texture", _texture);
        input.read("sleeper", _sleeper);

        input.read("fwdPoint", _fwdPoint);
        input.read("bwdPoint", _bwdPoint);

        input.read("track", _track);

        updateSpline();
    }

    void SplineTrajectory::write(vsg::Output& output) const
    {
        Group::write(output);

        output.write("sleepersDistance", _sleepersDistance);
        output.write("gaudge", _gaudge);
        output.write("autoPositioned", _autoPositioned);
        output.write("points", _points);
        output.write("geometry", _geometry);
        output.write("uv1", _uv1);
        output.write("uv2", _uv2);
        output.write("texture", _texture);
        output.write("sleeper", _sleeper);

        output.write("fwdPoint", _fwdPoint);
        output.write("bwdPoint", _bwdPoint);

        output.write("track", _track);
    }

    vsg::dvec3 SplineTrajectory::getPosition(double x) const
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
        return InterpolatedPTM(std::move(pt)).calculated;
    }

    void SplineTrajectory::updateSpline()
    {
        std::vector<vsg::dvec3> points;
        std::vector<vsg::dvec3> tangents;

        auto front = _fwdPoint->getPosition();

        points.push_back(front);
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

        points.push_back(_bwdPoint->getPosition() - front);
        tangents.push_back(_bwdPoint->getTangent());

        _railSpline.reset(new InterpolationSpline(points, tangents));
    }

    void SplineTrajectory::recalculate()
    {
        updateSpline();

        auto partitionBoundaries = ArcLength::partition(*_railSpline, _sleepersDistance);

        auto front = _fwdPoint->getPosition();

        std::vector<InterpolatedPTM> derivatives(partitionBoundaries.size());
        std::transform(std::execution::par_unseq, partitionBoundaries.begin(), partitionBoundaries.end(), derivatives.begin(),
                       [railSpline=_railSpline, front](const double T)
        {
            return std::move(InterpolatedPTM(railSpline->getTangent(T), front));
        });

        _track = vsg::MatrixTransform::create(vsg::translate(front));
        std::for_each(derivatives.begin(), derivatives.end(), [ sleeper=_sleeper, group=_track](const InterpolatedPTM &ptcm)
        {
            auto transform = vsg::MatrixTransform::create(ptcm.calculated);
            transform->addChild(sleeper);
            group->addChild(transform);
        });

        //auto last = std::unique(std::execution::par, derivatives.begin(), derivatives.end()); //vectorization is not supported
        //derivatives.erase(last, derivatives.end());

        assignRails(createSingleRail(vsg::vec3(_gaudge / 2.0, 0.0, 0.0), derivatives));
        assignRails(createSingleRail(vsg::vec3(-_gaudge / 2.0, 0.0, 0.0), derivatives));

        updateAttached();
    }

    std::pair<vsg::DataList, vsg::ref_ptr<vsg::ushortArray>> SplineTrajectory::createSingleRail(const vsg::vec3 &offset, const std::vector<InterpolatedPTM> &derivatives) const
    {
        std::vector<std::vector<vsg::vec3>> vertexGroups(derivatives.size());

        std::transform(std::execution::par_unseq, derivatives.begin(), derivatives.end(), vertexGroups.begin(),
        [geometry=_geometry, offset](const InterpolatedPTM &ptcm)
        {
            auto fmat = static_cast<vsg::mat4>(ptcm.calculated);
            std::vector<vsg::vec3> out;

            for(const auto &vec : geometry)
                out.push_back(fmat * (vec + offset));

            return std::move(out);
        });

        auto vsize = vertexGroups.size() * _geometry.size();
        auto vertArray = vsg::vec3Array::create(vsize);
        auto texArray = vsg::vec2Array::create(vsize);
        auto vertIt = vertArray->begin();
        auto texIt = texArray->begin();

        bool flip = false;

        for(const auto &vertexGroup : vertexGroups)
        {
            auto uv = flip ? _uv2.begin() : _uv1.begin();
            auto end = flip ? _uv2.end() : _uv1.end();
            for(const auto &vertex : vertexGroup)
            {
                Q_ASSERT(vertIt != vertArray->end());
                Q_ASSERT(uv < end);
                *vertIt = vertex;
                *texIt = *uv;
                texIt++;
                uv++;
                vertIt++;
            }
            flip = !flip;
        }

        std::vector<uint16_t> indices;
        const auto next = static_cast<uint16_t>(_geometry.size());
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

        auto colorArray = vsg::vec4Array::create(vsize);
        std::fill(colorArray->begin(), colorArray->end(), vsg::vec4(1.0f, 0.0f, 1.0f, 1.0f));

        auto normalArray = vsg::vec3Array::create(vsize);
        std::fill(normalArray->begin(), normalArray->end(), vsg::vec3(1.0f, 0.0f, 0.0f));

        auto ind = vsg::ushortArray::create(indices.size());
        std::copy(indices.begin(), indices.end(), ind->begin());

        return std::make_pair(vsg::DataList{vertArray, normalArray, texArray}, ind);
    }

    void SplineTrajectory::assignRails(std::pair<vsg::DataList, vsg::ref_ptr<vsg::ushortArray>> data)
    {
        auto vid = vsg::VertexIndexDraw::create();

        vid->assignArrays(data.first);

        vid->assignIndices(data.second);
        vid->indexCount = static_cast<uint32_t>(data.second->size());
        vid->instanceCount = 1;

        vsg::StateInfo si;
        si.image = _texture;
        si.lighting = false;

        auto stateGroup = _builder->createStateGroup(si);

        stateGroup->addChild(vid);

        _builder->compile(stateGroup);

        _track->addChild(stateGroup);

        //_builder->compile(_track);
    }

    void SplineTrajectory::updateAttached()
    {
        auto computeTransform = [this](vsg::MatrixTransform& transform)
        {
            double coord = 0.0;
            transform.getValue(META_PROPERTY, coord);
            transform.matrix = getMatrixAt(coord);
        };
        LambdaVisitor<decltype (computeTransform), vsg::MatrixTransform> ct(computeTransform);
        Trajectory::accept(ct);
    }

    void SplineTrajectory::add(vsg::ref_ptr<RailPoint> rp)
    {
        SplineInverter<vsg::dvec3, double> inverter(*_railSpline);
        double t = inverter.findClosestT(rp->getPosition());
        auto index = static_cast<size_t>(std::floor(t)) + 1;
        auto it = _points.begin() + index;
        Q_ASSERT(it <= _points.end());
        _points.insert(it, rp);
        rp->trajectory = this;

        recalculate();
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

    SceneTrajectory::~SceneTrajectory() {}

    void SceneTrajectory::read(vsg::Input& input)
    {
        Node::read(input);

        std::string name;
        input.read("trajName", name);

        addChild(input.options->objectCache->get(TOPOLOGY_KEY).cast<Topology>()->trajectories.at(name));

    }

    void SceneTrajectory::write(vsg::Output& output) const
    {
        Node::write(output);

        auto trajectory = children.front();
        Q_ASSERT(trajectory);

        std::string name;
        trajectory->getValue(META_NAME, name);
        output.write("trajName", name);

    }
}
