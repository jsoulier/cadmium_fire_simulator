#include <ankerl/unordered_dense.h>
#include <taskflow/algorithm/for_each.hpp>
#include <taskflow/taskflow.hpp>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

#include "fire_model_coordinator_event_driven.hpp"
#include "fire_profile.hpp"

void FireModelCoordinatorEventDriven::SimulateInternal()
{
    auto forEach = [this]<typename Function>(const std::vector<size_t>& cells, const Function& function)
    {
        static constexpr size_t kParallelThreshold = 256;
        static constexpr size_t kChunkSize = 16;
        if (cells.size() >= kParallelThreshold)
        {
            tf::Taskflow taskflow;
            taskflow.for_each_index(size_t(0), cells.size(), size_t(1), [&](size_t i)
            {
                function(cells[i]);
            }, tf::DynamicPartitioner(kChunkSize));
            Executor.run(taskflow).wait();
        }
        else
        {
            for (size_t cell : cells)
            {
                function(cell);
            }
        }
    };

    FireProfileTag();
    const std::vector<std::shared_ptr<cadmium::AbstractSimulator>>& subcomponents = TopCoordinator->getSubcomponents();
    const size_t cellCount = subcomponents.size();
    ankerl::unordered_dense::map<const cadmium::Component*, size_t> cellToIndex;
    cellToIndex.reserve(cellCount);
    for (size_t i = 0; i < cellCount; i++)
    {
        cellToIndex[subcomponents[i]->getComponent().get()] = i;
    }
    // inCouplings are sending to the current cell, outCellIndices are indices for the cells it sends to
    std::vector<std::vector<OneToOneCoupling>> inCouplings(cellCount);
    std::vector<std::vector<size_t>> outCellIndices(cellCount);
    for (const auto& [portTo, portsFrom] : Couplings)
    {
        const size_t destinationCellIndex = cellToIndex.at(portTo->getParent());
        for (const auto& portFrom : portsFrom)
        {
            const size_t sourceCellIndex = cellToIndex.at(portFrom->getParent());
            inCouplings[destinationCellIndex].emplace_back(portTo, portFrom);
            outCellIndices[sourceCellIndex].push_back(destinationCellIndex);
        }
    }
    // frame 1: all cells need to be computed
    // frame 2: cells that received messages from inCouplings need to be computed (where inCouplings comes from current or neighbors)
    // frame 3: repeat frame 2
    std::vector<size_t> cellIndices1(cellCount);
    std::iota(cellIndices1.begin(), cellIndices1.end(), size_t(0));
    std::vector<size_t> cellIndices2;
    std::uint64_t dedupeIndicesFrame = 0;
    std::vector<size_t> dedupeIndices(cellCount, dedupeIndicesFrame);
    const auto tryQueue = [&cellIndices2, &dedupeIndices, &dedupeIndicesFrame](size_t cellIndex)
    {
        if (dedupeIndices[cellIndex] != dedupeIndicesFrame)
        {
            dedupeIndices[cellIndex] = dedupeIndicesFrame;
            cellIndices2.push_back(cellIndex);
        }
    };

    double currentTime = TopCoordinator->getTimeNext();
    while (currentTime < kMaxTime)
    {
        {
            FireProfileTagBlock("Collection");
            forEach(cellIndices1, [&](size_t cell)
            {
                subcomponents[cell]->collection(currentTime);
            });
        }
        {
            FireProfileTagBlock("Propagate");
            forEach(cellIndices1, [&](size_t cell)
            {
                for (const auto& [portTo, portFrom] : inCouplings[cell])
                {
                    if (!portFrom->empty())
                    {
                        portTo->propagate(portFrom);
                    }
                }
            });
        }
        {
            FireProfileTagBlock("Transition");
            forEach(cellIndices1, [&](size_t cell)
            {
                subcomponents[cell]->transition(currentTime);
                subcomponents[cell]->clear();
            });
        }
        {
            FireProfileTagBlock("TimeNext");
            double nextTime = kMaxTime;
            for (size_t cell : cellIndices1)
            {
                nextTime = std::min(nextTime, subcomponents[cell]->getTimeNext());
            }

            dedupeIndicesFrame++;
            cellIndices2.clear();
            for (size_t cellIndex : cellIndices1)
            {
                const double cellTime = subcomponents[cellIndex]->getTimeNext();
                if (cellTime >= kMaxTime)
                {
                    continue;
                }
                tryQueue(cellIndex);
                if (cellTime == nextTime)
                {
                    for (size_t neighborIndex : outCellIndices[cellIndex])
                    {
                        tryQueue(neighborIndex);
                    }
                }
            }
            cellIndices1.swap(cellIndices2);
            LastTime = currentTime;
            currentTime = nextTime;
        }
        FireProfilePlot("CellCount", int64_t(cellIndices1.size()));
        FireProfileFrame();
    }
}
