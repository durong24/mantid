#ifndef MANTID_DATAHANDLING_LOADNEXUS_H_
#define MANTID_DATAHANDLING_LOADNEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/DataHandlingCommand.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
	
namespace Kernel
{
  class Logger;
}

  namespace DataHandling
  {
    /** @class LoadNeXus LoadNeXus.h MantidDataHandling/LoadNeXus.h

    Loads a file in NeXus format and stores it in a 2D workspace 
    (Workspace2D class). LoadNeXus is an algorithm and as such inherits
    from the Algorithm class, via DataHandlingCommand, and overrides
    the init() & exec() methods.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input RAW file </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the imported data </LI>
    </UL>

    @author Freddie Akeroyd, STFC ISIS Facility, GB
    @date 24/01/2008

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratories

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport LoadNeXus : public DataHandlingCommand
    {
    public:
      /// Default constructor
      LoadNeXus();

      /// Destructor
      ~LoadNeXus() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadNexus";};
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 1;};

    private:

      /// Overwrites Algorithm method.
      void init();

      /// Overwrites Algorithm method
      void exec();

      /// The name and path of the input file
      std::string m_filename;

      /// Pointer to the local workspace
      DataObjects::Workspace2D_sptr m_localWorkspace;

      ///static reference to the logger class
      static Kernel::Logger& g_log;

    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADNEXUS_H_*/
