/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "Logger.h"
#include <log4cxx/logmanager.h>
#include <log4cxx/defaultconfigurator.h>
#include <log4cxx/basicconfigurator.h>
#include <vector>

namespace Util
{

// Retrieve the \c name logger pointer.
// Configure Log4cxx on the first call.
auto getLogger(const std::string& name) -> LoggerPtr {
	using namespace log4cxx;
	static struct log4cxx_initializer {
		log4cxx_initializer() {
			// Check every second for a configuration file change
			DefaultConfigurator::setConfigurationWatchSeconds(1);

			// Look for a configuration file in the current working directory
			// and the same directory as the program
			std::vector<LogString> paths
				{ LOG4CXX_STR(".")
				, LOG4CXX_STR("${PROGRAM_FILE_PATH.PARENT_PATH}")
				};
			std::vector<LogString> names
				{ LOG4CXX_STR("${PROGRAM_FILE_PATH.STEM}.xml")
				, LOG4CXX_STR("${PROGRAM_FILE_PATH.STEM}.properties")
				};
			auto status       = spi::ConfigurationStatus::NotConfigured;
			auto selectedPath = LogString();
			std::tie(status, selectedPath) = DefaultConfigurator::configureFromFile(paths, names);
			if (status == spi::ConfigurationStatus::NotConfigured)
				BasicConfigurator::configure(); // Send events to the console
		}
		~log4cxx_initializer() {
			LogManager::shutdown();
		}
	} initialiser;
	return name.empty()
		? LogManager::getRootLogger()
		: LogManager::getLogger(name);
}

} // namespace Util
