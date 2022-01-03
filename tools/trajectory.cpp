#include    "trajectory.h"

#include    <QFile>
#include    <QDir>
#include    <QTextStream>
#include    <execution>
#include    <vsg/io/read.h>
#include    "topology.h"
#include    "sceneobjects.h"
#include    <vsg/all.h>


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

namespace route
{
    SplineTrajectory::SplineTrajectory(std::string name, vsg::ref_ptr<vsg::Builder> builder,
                                       std::vector<vsg::ref_ptr<SplinePoint>> points,
                                       std::vector<vsg::vec3> geometry,
                                       vsg::ref_ptr<vsg::Node> sleeper, double distance)
      : vsg::Inherit<Trajectory, SplineTrajectory>(name)
      , _builder(builder)
      , _geometry(geometry)
      , _sleeper(sleeper)
      , _sleepersDistance(distance)
    {
        _points = points;
        recalculate();
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
        /*
        Object::read(input);

        input.read("sections", sections);
        input.read("lenght", lenght);
        input.read("frontReversed", frontReversed);
        input.read("matrixStack", matrixStack);

        input.read("fwd", fwdTraj);
        */
    }

    void SplineTrajectory::write(vsg::Output& output) const
    {
        /*
        Object::write(output);

        output.write("sections", sections);
        output.write("lenght", lenght);
        output.write("frontReversed", frontReversed);
        output.write("matrixStack", matrixStack);

        output.write("fwd", fwdTraj);
        */
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
        return InterpolatedPTCM(_railSpline->getCurvature(T)).calculated;
    }

    void SplineTrajectory::recalculate()
    {
        std::vector<vsg::dvec3> points;
        auto front = _points.front()->getPosition();
        std::transform(_points.begin(), _points.end(), std::back_insert_iterator(points),
                       [front](const vsg::ref_ptr<SplinePoint> sp)
        {
            return sp->getPosition() - front;
        });

        _railSpline.reset(new CubicHermiteSpline<vsg::dvec3, double>(points));

        auto partitionBoundaries = ArcLength::partition(*_railSpline, _sleepersDistance);

        std::vector<InterpolatedPTCM> derivatives(partitionBoundaries.size());
        std::transform(std::execution::par_unseq, partitionBoundaries.begin(), partitionBoundaries.end(), derivatives.begin(),
                       [railSpline=_railSpline](const double T)
        {
            return std::move(InterpolatedPTCM(railSpline->getCurvature(T)));
        });

        //std::mutex m;
        _track = vsg::MatrixTransform::create(vsg::translate(front));
        std::for_each(derivatives.begin(), derivatives.end(), [ sleeper=_sleeper, group=_track](const InterpolatedPTCM &ptcm)
        {
            auto transform = vsg::MatrixTransform::create(ptcm.calculated);
            transform->addChild(sleeper);
            //std::lock_guard<std::mutex> guard(m);
            group->addChild(transform);
        });

        //auto last = std::unique(std::execution::par, derivatives.begin(), derivatives.end()); //vectorization is not supported
        //derivatives.erase(last, derivatives.end());

        auto vertices = std::transform_reduce(std::execution::par, derivatives.begin(), derivatives.end(), std::vector<vsg::vec3>(),
        [](auto vertices, auto transformed)
        {
            vertices.insert(std::end(vertices), std::begin(transformed), std::end(transformed));
            return vertices;
        },
        [geometry=_geometry](const InterpolatedPTCM &ptcm)
        {
            vsg::vec3 offset(1.0, 0.0, 0.0);

            auto fmat = static_cast<vsg::mat4>(ptcm.calculated);
            auto copy = geometry;

            for(auto vec = copy.begin(); vec != copy.end(); ++vec)
            {
                *vec = fmat * (*vec + offset);
            }

            return copy;
        });


        std::vector<uint16_t> indices;
        auto next = static_cast<uint16_t>(_geometry.size());
        uint16_t max = vertices.size() - next - 1;

        for (uint16_t ind = 0; ind < max; ++ind)
        {

            indices.push_back(ind);
            indices.push_back(ind + next);
            indices.push_back(ind + next + 1);
            indices.push_back(ind + next + 1);
            indices.push_back(ind + 1);
            indices.push_back(ind);
        }


        auto vertArray = vsg::vec3Array::create(vertices.size());
        std::copy(vertices.begin(), vertices.end(), vertArray->begin());
        /*
        auto colorArray = vsg::vec3Array::create(vertices.size());
        std::fill(colorArray->begin(), colorArray->end(), vsg::vec3(1.0f, 1.0f, 1.0f));
        auto texArray = vsg::vec3Array::create(vertices.size());
        std::fill(texArray->begin(), texArray->end(), vsg::vec3(0.0f, 0.0f, 0.0f));
        */
        auto ind = vsg::ushortArray::create(indices.size());
        std::copy(indices.begin(), indices.end(), ind->begin());

        assignRails(vsg::DataList{vertArray}, ind);
    }

    void SplineTrajectory::assignRails(vsg::DataList list, vsg::ref_ptr<vsg::ushortArray> indices)
    {
        auto stateGroup = _builder->createStateGroup();

        stateGroup->addChild(vsg::BindVertexBuffers::create(0, list));
        stateGroup->addChild(vsg::BindIndexBuffer::create(indices));
        stateGroup->addChild(vsg::DrawIndexed::create(indices->size(), 1, 0, 0, 0));

        _builder->compile(stateGroup);

        _track->addChild(stateGroup);
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
