#ifndef MANTID_CUSTOMINTERFACES_ALCBASELINEMODELLINGMODEL_H_
#define MANTID_CUSTOMINTERFACES_ALCBASELINEMODELLINGMODEL_H_

#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/DllConfig.h"

#include "MantidQtCustomInterfaces/Muon/IALCBaselineModellingModel.h"

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{
  /** ALCBaselineModellingModel : Concrete ALC Baseline Modelling step model implementation.
    
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport ALCBaselineModellingModel : public IALCBaselineModellingModel
  {
  public:
    /// @see IALCBaselineModellingModel::setData
    void setData(MatrixWorkspace_const_sptr data) { m_data = data; }
    /// @see IALCBaselineModellingModel::data
    MatrixWorkspace_const_sptr data() const { return m_data; }

    /// @see IALCBaselineModellingModel::fit
    void fit(IFunction_const_sptr function, const std::vector<Section> &sections);

    /// @see IALCBaselineModellingModel::fittedFunction
    IFunction_const_sptr fittedFunction() const { return m_fittedFunction; }

    /// @see IALCBaselineModellingModel::correctedData
    MatrixWorkspace_const_sptr correctedData() const { return m_correctedData; }

  private:
    /// Data to use for fitting
    MatrixWorkspace_const_sptr m_data;

    /// Corrected data of the last fit
    MatrixWorkspace_const_sptr m_correctedData;

    /// Result function of the last fit
    IFunction_const_sptr m_fittedFunction;

    /// Disables points which shouldn't be used for fitting
    static void disableUnwantedPoints(MatrixWorkspace_sptr ws, const std::vector<Section>& sections);
  };

} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTID_CUSTOMINTERFACES_ALCBASELINEMODELLINGMODEL_H_ */
