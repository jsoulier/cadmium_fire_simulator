#include <taskflow/algorithm/for_each.hpp>
#include <taskflow/algorithm/reduce.hpp>

#include <memory>
#include <vector>

#include "fire_model_coordinator_brute_force.hpp"
#include "fire_profile.hpp"

void FireModelCoordinatorBruteForce::SimulateInternal()
{
    const std::vector<std::shared_ptr<cadmium::AbstractSimulator>>& subcomponents = TopCoordinator->getSubcomponents();
    double currentTime = TopCoordinator->getTimeNext();
    double nextTime = kMaxTime;
    tf::Taskflow step;
    tf::Task collection = step.for_each_index(size_t(0), subcomponents.size(), size_t(1), [&](size_t i)
    {
        subcomponents[i]->collection(currentTime);
    });
    tf::Task propagate = step.for_each_index(size_t(0), Couplings.size(), size_t(1), [&](size_t i)
    {
        for (const auto& portFrom : Couplings[i].second)
        {
            if (!portFrom->empty())
            {
                Couplings[i].first->propagate(portFrom);
            }
        }
    });
    tf::Task transition = step.for_each_index(size_t(0), subcomponents.size(), size_t(1), [&](size_t i)
    {
        subcomponents[i]->transition(currentTime);
        subcomponents[i]->clear();
    });
    tf::Task timeNext = step.transform_reduce(
        subcomponents.begin(), subcomponents.end(), nextTime,
        [](double lhs, double rhs) -> double
        {
            return lhs < rhs ? lhs : rhs;
        },
        [](const std::shared_ptr<cadmium::AbstractSimulator>& simulator) -> double
        {
            return simulator->getTimeNext();
        });
    collection.precede(propagate);
    propagate.precede(transition);
    transition.precede(timeNext);
    tf::Taskflow driver;
    driver.emplace([&]()
    {
        while (currentTime < kMaxTime)
        {
            nextTime = kMaxTime;
            Executor.corun(step);
            LastTime = currentTime;
            currentTime = nextTime;
            FireProfileFrame();
        }
    });
    Executor.run(driver).wait();
}
