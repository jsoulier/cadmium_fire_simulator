/**
 * Abstract Cell-DEVS coupled model.
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2021-present Román Cárdenas Rodríguez
 * ARSLab - Carleton University
 * GreenLSI - Polytechnic University of Madrid
 */


#ifndef CADMIUM_CELLDEVS_CORE_COUPLED_HPP_
#define CADMIUM_CELLDEVS_CORE_COUPLED_HPP_

#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#if 1 // [wildfire_simulation]
#include <ankerl/unordered_dense.h>
#endif
#include <utility>
#include "cell.hpp"
#include "config.hpp"
#include "../../core/modeling/coupled.hpp"
#include "../../core/exception.hpp"
#if 1 // [wildfire_simulation]
#include <cadmium/wildfire_simulation/profile.hpp>
#endif

namespace cadmium::celldevs {
	/**
	 * @brief Abstract base class for coupled Cell-DEVS models.
	 * @tparam C the type used for representing a cell ID.
	 * @tparam S the type used for representing a cell state.
	 * @tparam V the type used for representing a neighboring cell's vicinities.
	 */
	template<typename C, typename S, typename V>
	class CellDEVSCoupled : public Coupled {
	 protected:
		nlohmann::json rawConfig;                                                           //!< JSON configuration file.
#if 0 // [cadmium_v2]
		std::unordered_map<std::string, std::shared_ptr<CellConfig<C, S, V>>> cellConfigs;  //!< unordered map with all the different configurations.
#endif
#if 1 // [wildfire_simulation]
		ankerl::unordered_dense::map<std::string, std::shared_ptr<CellConfig<C, S, V>>> cellConfigs;  //!< unordered map with all the different configurations.
#endif
	 public:
		CellDEVSCoupled(const std::string& id, const std::string& configFilePath): Coupled(id), rawConfig(), cellConfigs() {
			std::ifstream i(configFilePath);
			i >> rawConfig;
		}

#if 1 // [wildfire_simulation]
		CellDEVSCoupled(const std::string& id, nlohmann::json config): Coupled(id), rawConfig(std::move(config)), cellConfigs() {}

#endif
		/**
	     * Generates a cell configuration struct from a JSON object.
		 * @param configId ID of the configuration to be loaded.
		 * @param cellConfig cell configuration parameters (JSON object).
		 * @return pointer to the cell configuration created.
		 */
		virtual std::shared_ptr<CellConfig<C, S, V>> loadCellConfig(const std::string& configId, const nlohmann::json& cellConfig) const = 0;

		/**
		 * It adds all the cells according to the provided JSON configuration file.
		 * @param cellConfig pointer to cell configuration parameters.
		 */
		virtual void addCells(const std::shared_ptr<CellConfig<C, S, V>>& cellConfig) = 0;

		/**
		 * It adds all the default cells according to the provided JSON configuration file.
		 * @param defaultConfig pointer to the default cell configuration parameters.
		 */
		virtual void addDefaultCells(const std::shared_ptr<CellConfig<C, S, V>>& defaultConfig) {}

		//! It builds the Cell-DEVS model completely
		void buildModel() {
#if 1 // [wildfire_simulation]
			CADMIUM_PROFILE_TAG;
#endif
			loadCellConfigs();
			addCells();
			addCouplings();
#if 1 // [wildfire_simulation]
			rawConfig = {};
			cellConfigs = {};
#endif
		}

		//! It reads the provided JSON file to load all the defined cell configuration structures.
		void loadCellConfigs() {
#if 0 // [cadmium_v2]
			auto configs = rawConfig.contains("cells") ? rawConfig["cells"] : nlohmann::json::object();
			auto rawDefault = (configs.contains("default")) ? configs["default"] : nlohmann::json::object();
			auto defaultConfig = this->loadCellConfig("default", rawDefault);
			cellConfigs["default"] = defaultConfig;
#endif
#if 1 // [wildfire_simulation]
			CADMIUM_PROFILE_TAG;
			static const nlohmann::json object = nlohmann::json::object();
			const nlohmann::json& configs = rawConfig.contains("cells") ? rawConfig["cells"] : object;
			const nlohmann::json& rawDefault = configs.contains("default") ? configs["default"] : object;
			cellConfigs["default"] = this->loadCellConfig("default", rawDefault);
#endif
			for (auto const&[configId, patch]: configs.items()) {
				if (configId != "default") {
					cellConfigs[configId] = loadCellConfig(configId, rawDefault, patch);
				}
			}
		}

		//! It adds all the cells according to the provided JSON configuration file.
		void addCells() {
#if 1 // [wildfire_simulation]
			CADMIUM_PROFILE_TAG;
#endif
			for (auto const&[configId, cellConfig]: cellConfigs) {
				if (configId != "default") {  // Default cells are added at the end of this method
					this->addCells(cellConfig);
				}
			}
			this->addDefaultCells(cellConfigs.at("default"));
		}

		/**
		 * It generates a cell configuration parameter from a default configuration and a patch.
		 * @param configId ID of the cell-DEVS configuration.
		 * @param cellConfig default configuration (JSON object).
		 * @param patch patch to be applied to the default configuration (JSON object).
		 * @return pointer to the created cell configuration struct.
		 */
		std::shared_ptr<CellConfig<C, S, V>> loadCellConfig(const std::string& configId, const nlohmann::json& cellConfig, const nlohmann::json& patch) const {
#if 0 // [cadmium_v2]
			auto copyConfig = nlohmann::json::parse(cellConfig.dump());
			copyConfig.merge_patch(patch);
			return this->loadCellConfig(configId, copyConfig);
#endif
#if 1 // [wildfire_simulation]
			nlohmann::json cellConfigCopy = cellConfig;
			cellConfigCopy.merge_patch(patch);
			return this->loadCellConfig(configId, cellConfigCopy);
#endif
		}

		//! It adds all the couplings required in the scenario according to the configuration file.
		void addCouplings() {
#if 1 // [wildfire_simulation]
			CADMIUM_PROFILE_TAG;
#endif
			for (const auto& [_, cell]: Coupled::components) {
				auto cellModel = std::dynamic_pointer_cast<Cell<C, S, V>>(cell);
				if (cellModel == nullptr) {
					throw CadmiumModelException("Scenario component is not a cell");
				}
				for (const auto& neighbor: cellModel->getNeighborhood()) {
#if 0 // [cadmium_v2]
					addIC(cellId(neighbor.first), "outputNeighborhood", cellModel->getId(), "inputNeighborhood");
#endif
#if 1 // [wildfire_simulation]
					const std::string neighborId = cellId(neighbor.first);
					// since non-burnable cells are omitted, the neighborhood can have references to non-existing cells
					if (Coupled::components.find(neighborId) == Coupled::components.end()) {
						continue;
					}
					addIC(neighborId, "outputNeighborhood", cellModel->getId(), "inputNeighborhood");
#endif
				}
				auto cellConfig = cellModel->getCellConfig();
				for (const auto& [portFrom, portTo]: cellConfig->EIC) {
					addDynamicEIC(portFrom, cellModel->getId(), portTo);  // TODO
				}
				for (const auto& portTo: cellConfig->EOC) {
					addDynamicEOC(cellModel->getId(), "outputNeighborhood", portTo);  // TODO
				}
#if 1 // [wildfire_simulation]
				cellModel->freeCellConfig();
#endif
			}
		}

		/**
		 * Returns a string representation of a cell.
		 * @param id ID of a cell.
		 * @return string representation of the cell ID.
		 */
		static std::string cellId(const C& id) {
#if 0 // [cadmium_v2]
			std::stringstream ss;
#endif
#if 1 // [wildfire_simulation]
			thread_local std::ostringstream ss;
			ss.str("");
			ss.clear();
#endif
			ss << id;
			return ss.str();
		}
	};
} // namespace cadmium::celldevs

#endif // CADMIUM_CELLDEVS_CORE_COUPLED_HPP_
