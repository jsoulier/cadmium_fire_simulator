/**
 * Utility stuff for vectors. These are required for grid-based Cell-DEVS scenarios.
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2022-present Román Cárdenas Rodríguez
 * ARSLab - Carleton University
 * GreenLSI - Polytechnic University of Madrid
 */


#ifndef CADMIUM_CELLDEVS_GRID_UTILILITY_HPP_
#define CADMIUM_CELLDEVS_GRID_UTILILITY_HPP_

#if 0 // [cadmium_v2]
#include <cstddef>
#endif
#if 1 // [wildfire_simulation]
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#endif
#include <iostream>
#if 0 // [cadmium_v2]
#include <string>
#include <vector>
#endif
#if 1 // [wildfire_simulation]
#include <nlohmann/json.hpp>
#endif

namespace cadmium::celldevs {
#if 0 // [cadmium_v2]
	using coordinates = std::vector<int>;  //!< Type alias for referring to cell coordinates
#endif
#if 1 // [wildfire_simulation]
	using coordinates = glm::ivec2;  //!< Type alias for referring to cell coordinates
#endif

	/**
	 * Auxiliary function for printing cell coordinates.
	 * @param os output stream.
	 * @param v coordinates to be printed.
	 * @return output stream containing the printed values of the coordinates.
	*/
#if 0 // [cadmium_v2]
	std::ostream &operator<<(std::ostream &os, const coordinates & v) {
		os << "(";
		std::string separator;
		for (const auto &x : v) {
			os << separator << x;
			separator = ",";
		}
		os << ")";
#endif
#if 1 // [wildfire_simulation]
	inline std::ostream &operator<<(std::ostream &os, const coordinates & v) {
		os << v.x << "," << v.y;
#endif
		return os;
	}
}  //namespace cadmium::celldevs

#if 0 // [cadmium_v2]
/**
 * @brief Auxiliary hash container for vectors.
 *
 * It allows us to use vectors as keys in hash maps.
 * @tparam T vector content type.
 */
template <typename T>
struct std::hash<std::vector<T>> {
	/**
	 * Hashing function for vectors.
	 * This function is based on https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector
	 * @param vec vector to be used by the hashing function.
	 * @return hash resulting from the vector.
	 */
	std::size_t operator()(const std::vector<T>& vec) const {
		std::size_t seed = vec.size();
		for(const auto &i: vec) {
			seed ^= hash<T>()(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
#endif
#if 1 // [wildfire_simulation]
namespace nlohmann {
	template <>
	struct adl_serializer<cadmium::celldevs::coordinates> {
		static void from_json(const json& j, cadmium::celldevs::coordinates& value) {
			value.x = j.at(0).get<int>();
			value.y = j.at(1).get<int>();
#endif
		}
#if 0 // [cadmium_v2]
		return seed;
	}
};
#endif
#if 1 // [wildfire_simulation]
	};
}  // namespace nlohmann
#endif

#endif //CADMIUM_CELLDEVS_GRID_UTILILITY_HPP_
