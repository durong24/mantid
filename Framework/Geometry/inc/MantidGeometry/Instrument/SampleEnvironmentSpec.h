#ifndef MANTID_GEOMETRY_SAMPLEENVIRONMENTSPEC_H_
#define MANTID_GEOMETRY_SAMPLEENVIRONMENTSPEC_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/Can.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace Mantid {
namespace Geometry {

/**
  Defines the properties of a named SampleEnvironment setup. It is used
  in conjunction with the SampleEnvionmentBuilder to construct given
  configuration of SampleEnvironment. It can be read from XML by a
  SampleEnvironmentParser.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_GEOMETRY_DLL SampleEnvironmentSpec {
public:
  // Convenience typedefs
  using CanIndex = std::unordered_map<std::string, Can_const_sptr>;
  using ComponentList = std::vector<Object_const_sptr>;

  SampleEnvironmentSpec(std::string name);

  /// @return The name of the specification
  inline const std::string &name() const { return m_name; }
  /// @return The number of known cans
  inline size_t ncans() const { return m_cans.size(); }
  /// @return The number of non-can components
  inline size_t ncomponents() const { return m_components.size(); }
  Can_const_sptr findCan(const std::string &id) const;

  std::unique_ptr<SampleEnvironment>
  buildEnvironment(const std::string &canID) const;

  void addCan(const Can_const_sptr &can);
  void addComponent(const Object_const_sptr &component);

private:
  std::string m_name;
  CanIndex m_cans;
  ComponentList m_components;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_SAMPLEENVIRONMENTSPEC_H_ */
