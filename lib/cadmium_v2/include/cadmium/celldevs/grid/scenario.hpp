/**
 * Cell-DEVS grid scenario properties.
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2021-present Román Cárdenas Rodríguez
 * ARSLab - Carleton University
 * GreenLSI - Polytechnic University of Madrid
 */


#ifndef CADMIUM_CELLDEVS_GRID_SCENARIO_HPP_
#define CADMIUM_CELLDEVS_GRID_SCENARIO_HPP_

#include <algorithm>
#include <cmath>
#if 1 // [wildfire_simulation]
#include <climits>
#endif
#include <numeric>
#include <utility>
#include <vector>
#include "utility.hpp"
#include "../../core/exception.hpp"

namespace cadmium::celldevs {
	/**
	 * @brief Auxiliary struct for grid-like Cadmium Cell-DEVS models.
	 *
	 * This struct contains useful functions for these scenarios (e.g., computing distances between cells).
	 */
	struct GridScenario {
		const coordinates shape;   //!< shape of the scenarios (i.e., how many cells are in the scenario by dimension).
		const coordinates origin;  //!< coordinates of the origin cell. In 2D scenarios, it corresponds to the upper-left cell.
		const bool wrapped;        //!< if true, the scenario is wrapped.

#if 1 // [wildfire_simulation]
	 private:
		// deduped moore neighborhoods to save memory (keyed by range)
		mutable std::vector<std::pair<int, std::vector<coordinates>>> mooreNeighborhoods;

	 public:
#endif
		/**
		 * Grid scenario constructor function.
		 * @param shape shape of the scenario. It must have at least one dimension, and its values must be greater than 0.
		 * @param origin coordinates of the origin cell. The size must be equal to the size of the shape.
		 * @param shape shape of the scenario. Its values must be greater than 0.
		 * @param origin coordinates of the origin cell.
		 * @param wrapped it determines whether or not the grid Cell-DEVS scenario is wrapped.
		 */
		GridScenario(const coordinates& shape, const coordinates& origin, bool wrapped): shape(shape), origin(origin), wrapped(wrapped) {
#if 0 // [cadmium_v2]
			if (shape.empty()) {
				throw CadmiumModelException("invalid scenario shape");
			}
			if (std::any_of(shape.begin(), shape.end(), [](int a) { return a < 1; })) {
				throw CadmiumModelException("invalid scenario shape");
			}
			if (shape.size() != origin.size()) {
				throw CadmiumModelException("shape-origin dimension mismatch");
#endif
#if 1 // [wildfire_simulation]
			for (glm::length_t i = 0; i < coordinates::length(); ++i) {
				if (shape[i] < 1) {
					throw CadmiumModelException("invalid scenario shape");
				}
#endif
			}
		}

		/**
		 * It checks if a given cell is inside the scenario.
		 * @param cell coordinates of the cell under study.
		 * @return true if cell coordinates are within the scenario.
		 */
		[[nodiscard]] bool cellInScenario(const coordinates& cell) const {
#if 0 // [cadmium_v2]
			return cell.size() == shape.size() && std::all_of(cell.begin(), cell.end(), [this, i = 0](int a) mutable {
				a = a - origin[i];
				return a >= 0 && a < shape[i++];
			});
#endif
#if 1 // [wildfire_simulation]
			for (glm::length_t i = 0; i < coordinates::length(); ++i) {
				int a = cell[i] - origin[i];
				if (a < 0 || a >= shape[i]) {
					return false;
				}
			}
			return true;
#endif
		}

		/**
		 * It checks if a given distance vector makes sense in the scenario.
		 * @param distance distance vector under study.
		 * @return true if the distance vector may be valid in the scenario.
		 */
		[[nodiscard]] bool validDistance(const coordinates& distance) const {
#if 0 // [cadmium_v2]
			return distance.size() == shape.size() && std::all_of(distance.begin(), distance.end(), [this, i = 0](int a) mutable {
				return std::abs(a) < shape[i++];
			});
#endif
#if 1 // [wildfire_simulation]
			for (glm::length_t i = 0; i < coordinates::length(); ++i) {
				if (std::abs(distance[i]) >= shape[i]) {
					return false;
				}
			}
			return true;
#endif
		}

		/**
		 * It computes the distance vector that interconnects two cells.
		 * @param cellFrom coordinates of the origin cell.
		 * @param cellTo coordinates of the destination cell.
		 * @return a distance vector that describes how to go from cellFrom to cellTo.
		 */
		[[nodiscard]] coordinates distanceVector(const coordinates& cellFrom, const coordinates& cellTo) const {
			if (!cellInScenario(cellFrom) || !cellInScenario(cellTo)) {
				throw CadmiumModelException("Cell does not belong to scenario");
			}
			coordinates distance;
#if 0 // [cadmium_v2]
			for (int i = 0; i < shape.size(); ++i) {
#endif
#if 1 // [wildfire_simulation]
			for (glm::length_t i = 0; i < coordinates::length(); ++i) {
#endif
				int d = cellTo[i] - cellFrom[i];
				if (wrapped && abs(d) > shape[i] / 2) {
					d -= (int) copysign(shape[i], d);
				}
#if 0 // [cadmium_v2]
				distance.push_back(d);
#endif
#if 1 // [wildfire_simulation]
				distance[i] = d;
#endif
			}
			if (!validDistance(distance)) {
				throw CadmiumModelException("Invalid distance vector");
			}
			return distance;
		}

		/**
		 * From an origin cell and a distance vector, it computes the destination cell.
		 * @param cellFrom coordinates of the origin cell.
		 * @param distance distance vector between the origin and destination cells.
		 * @return coordinates of the destination cell.
		 */
		[[maybe_unused]] [[nodiscard]] coordinates cellTo(const coordinates& cellFrom, const coordinates& distance) const {
			if (!cellInScenario(cellFrom)) {
				throw CadmiumModelException("Cell does not belong to scenario");
			}
			if (!validDistance(distance)) {
				throw CadmiumModelException("Invalid distance vector");
			}
			coordinates cellTo;
#if 0 // [cadmium_v2]
			for (int i = 0; i < shape.size(); ++i) {
				auto v = cellFrom[i] + distance[i];
#endif
#if 1 // [wildfire_simulation]
			for (glm::length_t i = 0; i < coordinates::length(); ++i) {
				int v = cellFrom[i] + distance[i];
#endif
				if (wrapped) {
					v = (v + shape[i]) % shape[i];
				}
#if 0 // [cadmium_v2]
				cellTo.push_back(v);
#endif
#if 1 // [wildfire_simulation]
				cellTo[i] = v;
#endif
			}
			if (!cellInScenario(cellTo)) {
				throw CadmiumModelException("Cell does not belong to scenario");
			}
			return cellTo;
		}

		/**
		 * From a distance vector and a destination cell, it computes the origin cell.
		 * @param distance distance vector between the origin and destination cells.
		 * @param cellTo coordinates of the destination cell.
		 * @return coordinates of the origin cell.
		 */
		[[maybe_unused]] [[nodiscard]] coordinates cellFrom(const coordinates& distance, const coordinates& cellTo) const {
			if (!validDistance(distance)) {
				throw CadmiumModelException("Invalid distance vector");
			}
			if (!cellInScenario(cellTo)) {
				throw CadmiumModelException("Cell does not belong to scenario");
			}
			coordinates cellFrom;
#if 0 // [cadmium_v2]
			for (int i = 0; i < shape.size(); ++i) {
				auto v = cellTo[i] - distance[i];
#endif
#if 1 // [wildfire_simulation]
			for (glm::length_t i = 0; i < coordinates::length(); ++i) {
				int v = cellTo[i] - distance[i];
#endif
				if (wrapped) {
					v = (v + shape[i]) % shape[i];
				}
#if 0 // [cadmium_v2]
				cellFrom.push_back(v);
#endif
#if 1 // [wildfire_simulation]
				cellFrom[i] = v;
#endif
			}
			if (!cellInScenario(cellFrom)) {
				throw CadmiumModelException("Cell does not belong to scenario");
			}
			return cellFrom;
		}

		/**
		 * It computes the Minkowski distance between two cells (see https://en.wikipedia.org/wiki/Minkowski_distance)
		 * @param p order to be applied when computing the Minkowski distance. It must be greater than 0.
		 * @param cellFrom coordinates of the origin cell.
		 * @param cellTo coordinates of the destination cell.
		 * @return value of the Minkowski distance between the provided cells.
		 */
		[[nodiscard]] inline double minkowskiDistance(int p, const coordinates& cellFrom, const coordinates& cellTo) const {
			return minkowskiDistance(p, distanceVector(cellFrom, cellTo));
		}

		/**
		 * It computes the Manhattan distance between two cells (see https://en.wikipedia.org/wiki/Taxicab_geometry)
		 * @param cellFrom coordinates of the origin cell.
		 * @param cellTo coordinates of the destination cell.
		 * @return value of the Manhattan distance between the provided cells.
		 */
		[[nodiscard]] inline int manhattanDistance(const coordinates& cellFrom, const coordinates& cellTo) const {
			return manhattanDistance(distanceVector(cellFrom, cellTo));
		}

		/**
		 * It computes the Chebyshev distance between two cells (see https://en.wikipedia.org/wiki/Chebyshev_distance)
		 * @param cellFrom coordinates of the origin cell.
		 * @param cellTo coordinates of the destination cell.
		 * @return value of the Chebyshev distance between the provided cells.
		 */
		[[nodiscard]] inline int chebyshevDistance(const coordinates& cellFrom, const coordinates& cellTo) const {
			return chebyshevDistance(distanceVector(cellFrom, cellTo));
		}

		/**
		 * It computes the Minkowski distance from a distance vector.
		 * @param p order to be applied when computing the Minkowski distance. It must be greater than 0.
		 * @param distance distance vector between the origin and destination cells.
		 * @return value of the Minkowski distance corresponding to the provided distance vector.
		 */
		[[nodiscard]] static double minkowskiDistance(int p, const coordinates& distance) {
			if (p < 1) {
				throw CadmiumModelException("p must be greater than 0");
			}
#if 0 // [cadmium_v2]
			auto x = std::accumulate(distance.begin(), distance.end(), 0., [p](double sum, int v) { return sum + std::pow(std::abs(v), p); });
#endif
#if 1 // [wildfire_simulation]
			double x = 0.0;
			for (glm::length_t i = 0; i < coordinates::length(); ++i) {
				x += std::pow(std::abs(distance[i]), p);
			}
#endif
			return std::pow(x, 1.0 / p);
		}

		/**
		 * It computes the Manhattan distance from a distance vector.
		 * @param distance distance vector between the origin and destination cells.
		 * @return value of the Manhattan distance corresponding to the provided distance vector.
		 */
		[[nodiscard]] static inline int manhattanDistance(const coordinates& distance) {
#if 0 // [cadmium_v2]
			return std::accumulate(distance.begin(), distance.end(), 0, [](int sum, int v) { return sum + std::abs(v); });
#endif
#if 1 // [wildfire_simulation]
			int total = 0;
			for (glm::length_t i = 0; i < coordinates::length(); ++i) {
				total += std::abs(distance[i]);
			}
			return total;
#endif
		}

		/**
		 * It computes the Chebyshev distance from a distance vector.
		 * @param distance distance vector between the origin and destination cells.
		 * @return value of the Chebyshev distance corresponding to the provided distance vector.
		 */
		[[nodiscard]] static inline int chebyshevDistance(const coordinates& distance) {
#if 0 // [cadmium_v2]
			return *std::max_element(distance.begin(), distance.end(), [](int a, int b) { return std::abs(a) < std::abs(b); });
#endif
#if 1 // [wildfire_simulation]
			int maxValue = distance[0];
			for (glm::length_t i = 1; i < coordinates::length(); ++i) {
				if (std::abs(distance[i]) > std::abs(maxValue)) {
					maxValue = distance[i];
				}
			}
			return maxValue;
#endif
		}

		/**
		 * It returns a vector with all the distance vectors that describe a Moore neighborhood (see https://en.wikipedia.org/wiki/Moore_neighborhood).
		 * @param range desired range for the resulting Moore neighborhood. It must be greater than 0.
		 * @return vector of distance vectors that conform the Moore neighborhood.
		 */
#if 0 // [cadmium_v2]
		[[nodiscard]] std::vector<coordinates> mooreNeighborhood(int range) const {
#endif
#if 1 // [wildfire_simulation]
		[[nodiscard]] const std::vector<coordinates>& mooreNeighborhood(int range) const {
			for (const auto& [otherRange, otherNeighborhood]: mooreNeighborhoods) {
				if (otherRange == range) {
					return otherNeighborhood;
				}
			}
#endif
			std::vector<coordinates> neighborhood;
			for (const auto& distance: mooreScenario(range)) {
				if (validDistance(distance)) {
					neighborhood.push_back(distance);
				}
			}
#if 0 // [cadmium_v2]
			return neighborhood;
#endif
#if 1 // [wildfire_simulation]
			return mooreNeighborhoods.emplace_back(range, std::move(neighborhood)).second;
#endif
		}

		/**
		 * It returns a vector with all the distance vectors that describe a von Neumann neighborhood (see https://en.wikipedia.org/wiki/Von_Neumann_neighborhood).
		 * @param range desired range for the resulting Moore neighborhood. It must be greater than 0.
		 * @return vector of distance vectors that conform the von Neumann neighborhood.
		 */
		[[nodiscard]] std::vector<coordinates> vonNeumannNeighborhood(int range) const {
			std::vector<coordinates> neighborhood;
			for (const auto& distance: mooreScenario(range)) {
				if (validDistance(distance) && manhattanDistance(distance) <= range) {
					neighborhood.push_back(distance);
				}
			}
			return neighborhood;
		}

		/**
		 * It returns a vector with all the distance vectors which Minkowski distance is less than or equal to a given maximum distance.
		 * @param p order to be applied when computing the Minkowski distance. It must be greater than 0.
		 * @param maxDistance maximum Minkowski distance for the distance vectors.
		 * @return vector of distance vectors that conform the Minkowski neighborhood.
		 */
		[[nodiscard]] std::vector<coordinates> minkowskiNeighborhood(int p, double maxDistance) const {
			std::vector<coordinates> neighborhood;
			for (const auto& distance: mooreScenario((int) maxDistance)) {
				if (validDistance(distance) && minkowskiDistance(p, distance) <= maxDistance) {
					neighborhood.push_back(distance);
				}
			}
			return neighborhood;
		}

		/**
		 * It generates an unbiased grid scenario that mimics a Moore neighborhood.
		 * The coordinates of each cell in the scenario can be considered as a distance vector.
		 * @param range range of the Moore neighborhood. It must be greater than 0.
		 * @return Moore scenario of the desired range.
		 */
		[[nodiscard]] GridScenario mooreScenario(int range) const {
			if (range < 1) {
				throw CadmiumModelException("range must be greater than 0");
			}
#if 0 // [cadmium_v2]
			auto nShape = coordinates(shape.size(), 2 * range + 1);
			auto nOrigin = coordinates(shape.size(), -range);
#endif
#if 1 // [wildfire_simulation]
			auto nShape = coordinates(2 * range + 1);
			auto nOrigin = coordinates(-range);
#endif
			return {nShape, nOrigin, false};
		}

		//! Auxiliary class for iterating over all the cells within the grid scenario.
		class Iterator {
			const GridScenario *scenario;  //!< Pointer to the corresponding grid scenario.
			coordinates cell;         //!< latest cell coordinate computed by the iterator.

			/**
			 * It computes the coordinates of the next cell to be iterated.
			 * Once the iterator is done with all the cells in the scenario, it sets cell to an empty vector.
			 * @param d dimension being explored. Usually, it is set to 0 (i.e., the first dimension).
			 */
			void next(int d) {
#if 0 // [cadmium_v2]
				if (d < 0 || d >= cell.size()) {
					cell = {};
#endif
#if 1 // [wildfire_simulation]
				if (d >= coordinates::length()) {
					cell = coordinates(INT_MAX);
#endif
				} else if (cell[d] - scenario->origin[d] < scenario->shape[d] - 1) {
					cell[d]++;
				} else {
					cell[d] = scenario->origin[d];
					next(d + 1);
				}
			}

		 public:
			/**
			 * Iterator constructor.
			 * @param scenario pointer to the scenario being iterated.
			 * @param cell first cell coordinate (usually, the scenario origin cell).
			 */
			Iterator(const GridScenario *scenario, coordinates cell): scenario(scenario), cell(std::move(cell)) {
#if 0 // [cadmium_v2]
				if (!(this->cell.empty() || scenario->cellInScenario(this->cell))) {
#endif
#if 1 // [wildfire_simulation]
				if (this->cell != coordinates(INT_MAX) && !scenario->cellInScenario(this->cell)) {
#endif
					throw CadmiumModelException("Invalid iterator");
				}
			}

			//! Two iterators are different if the point to different scenarios or if the current cell is different.
			bool operator!=(const Iterator& b) const {
				return scenario != b.scenario || cell != b.cell;
			}

			//! when updating the iterator, we just call the next method and return the same iterator.
			Iterator& operator++() {
				next(0);
				return *this;
			}

			//! Iterators return the coordinates of the current cell.
			const coordinates& operator*() const {
				return cell;
			}
		};

		//! @return an iterator that initially points to the origin cell of the scenario.
		Iterator begin() {
			return {this, origin};
		}

		//! @return a consumed iterator (i.e., it initially points to an empty vector).
		Iterator end() {
#if 0 // [cadmium_v2]
			return {this, {}};
#endif
#if 1 // [wildfire_simulation]
			return {this, coordinates(INT_MAX)};
#endif
		}
	};
}

#endif //CADMIUM_CELLDEVS_GRID_SCENARIO_HPP_
