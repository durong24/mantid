//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadGSS.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/LoadAlgorithmFactory.h"

#include <boost/math/special_functions/fpclassify.hpp>
#include <Poco/File.h>
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadGSS)

  //register the algorithm into loadalgorithm factory
  DECLARE_LOADALGORITHM(LoadGSS)


  /// Sets documentation strings for this algorithm
  void LoadGSS::initDocs()
  {
    this->setWikiSummary("Loads a GSS file such as that saved by [[SaveGSS]]. This is not a lossless process, as SaveGSS truncates some data. There is no instrument assosciated with the resulting workspace.  '''Please Note''': Due to limitations of the GSS file format, the process of going from Mantid to a GSS file and back is not perfect. ");
    this->setOptionalMessage("Loads a GSS file such as that saved by SaveGSS. This is not a lossless process, as SaveGSS truncates some data. There is no instrument assosciated with the resulting workspace.  'Please Note': Due to limitations of the GSS file format, the process of going from Mantid to a GSS file and back is not perfect.");
  }

  /**
  * Initialise the algorithm
  */
  void LoadGSS::init()
  {
    std::vector<std::string> exts;
    exts.push_back(".gsa");
    exts.push_back(".txt");
    declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load, exts), "The input filename of the stored data");
    declareProperty(new API::WorkspaceProperty<>("OutputWorkspace", "", Kernel::Direction::Output));
  }

  /**
  * Execute the algorithm
  */
  void LoadGSS::exec()
  {
    using namespace Mantid::API;
    std::string filename = getPropertyValue("Filename");

    std::vector<MantidVec*> gsasDataX;
    std::vector<MantidVec*> gsasDataY;
    std::vector<MantidVec*> gsasDataE;

    std::ifstream input(filename.c_str(), std::ios_base::in);

    MantidVec* X = new MantidVec();
    MantidVec* Y = new MantidVec();
    MantidVec* E = new MantidVec();

    int nSpec = 0;

    Progress* prog = NULL;

    char currentLine[256];
    std::string wsTitle;
    std::string slogTitle;
    bool slogtitleset = false;
    char filetype = 'x';

    bool calslogx0 = true;
    double bc4 = 0;
    double bc3 = 0;

    bool db1 = true;

    bool multiplybybinwidth = false;
    bool multiplybybinwidthdefined = false;

    // Gather data
    if ( input.is_open() )
    {
      if ( ! input.eof() )
      {
        // Get workspace title (should be first line or 2nd line for SLOG)
        input.getline(currentLine, 256);
        wsTitle = currentLine;
      }
      while ( ! input.eof() && input.getline(currentLine, 256) )
      {
        if ( nSpec != 0 && prog == NULL )
        {
          prog = new Progress(this, 0.0, 1.0, nSpec);
        }
        if (!slogtitleset){
          slogTitle = currentLine;
          slogtitleset = true;
        }
        double bc1 = 0;
        double bc2 = 0;
        if (  currentLine[0] == '\n' || currentLine[0] == '#' )
        {
          // Comment line
          if ( nSpec == 0 || !multiplybybinwidthdefined)
          {
            int noSpectra = 0;
            std::string line;

            std::istringstream inputLine(currentLine, std::ios::in);
            inputLine.ignore(256, ' ');
            inputLine >> noSpectra >> line;

            if ( ( noSpectra != 0 ) && ( line == "Histograms" ) )
            {
              nSpec = noSpectra;
            } else if (line == "Multiplied"){
              if (noSpectra == 0)
                multiplybybinwidth = false;
              else
                multiplybybinwidth = true;
              multiplybybinwidthdefined = true;
              g_log.debug() << "Multiplied by bin width = " << noSpectra << "\n";
            }

          } // if nSpec
          continue;
        }
        else if ( currentLine[0] == 'B' )
        {
        	// Line start with Bank including file format, X0 information and etc.

          // 1. Save the previous to array and initialze new MantiVec for (X, Y, E)
          if ( X->size() != 0 )
          {
            gsasDataX.push_back(X);
            gsasDataY.push_back(Y);
            gsasDataE.push_back(E);
            X = new MantidVec();
            Y = new MantidVec();
            E = new MantidVec();

            if ( prog != NULL )
              prog->report();
          }

          // 2. Parse

          /* BANK <SpectraNo> <NBins> <NBins> RALF <BC1> <BC2> <BC1> <BC4>
           *    OR,
           * BANK <SpectraNo> <NBins> <NBins> SLOG <BC1> <BC2> <BC3> 0>
          *  BC1 = X[0] * 32
          *  BC2 = X[1] * 32 - BC1
          *  BC4 = ( X[1] - X[0] ) / X[0]
          */

          // Parse B-line
          int specno, nbin1, nbin2;
          std::istringstream inputLine(currentLine, std::ios::in);

          /*
          inputLine.ignore(256, 'F');
          inputLine >> bc1 >> bc2 >> bc1 >> bc4;
          */

          inputLine.ignore(256, 'K');
          std::string filetypestring;

          inputLine >> specno >> nbin1 >> nbin2 >> filetypestring;
          g_log.debug() << "filetypestring = " << filetypestring << std::endl;

          if (filetypestring[0] == 'S'){
            // SLOG
        	  filetype = 's';
        	  inputLine >> bc1 >> bc2 >> bc3 >> bc4;
          } else if (filetypestring[0] == 'R'){
            // RALF
        	  filetype = 'r';
        	  inputLine >> bc1 >> bc2 >> bc1 >> bc4;
          } else {
        	  std::cout << "Unsupported File Type: " << filetypestring << std::endl;
        	  std::cout << "Returned with error!\n";
        	  return;
          }

          // Determine x0
          double x0 = 0;
          if (filetype == 'r'){
        	  x0 = bc1 / 32;
            g_log.debug() << "RALF: x0 = " << x0 << "  bc4 = " << bc4 << std::endl;
            X->push_back(x0);
          } else {
            // Cannot calculate x0, turn on the flag
            calslogx0 = true;
          }
        }
        else
        {
          double xValue;
          double yValue;
          double eValue;

          double xPrev;

          // * Get previous X value
          if ( X->size() != 0 )
          {
            xPrev = X->back();
          }
          else if (filetype == 'r'){
            // Except if RALF
            throw Mantid::Kernel::Exception::NotImplementedError("LoadGSS: File was not in expected format.");
          } else {
              xPrev = -0.0;
          }

          std::istringstream inputLine(currentLine, std::ios::in);
          inputLine >> xValue >> yValue >> eValue;

          // It is different for the definition of X, Y, Z in SLOG and RALF format
          if (filetype == 'r'){
            // RALF
            double tempy = yValue;

        	  xValue = (2 * xValue) - xPrev;
        	  yValue = yValue / ( xPrev * bc4 );
        	  eValue = eValue / ( xPrev * bc4 );

            if (db1){
              g_log.debug() << "Type: " << filetype << "  xPrev = " << xPrev << " bc4 =" << bc4 << std::endl;
              g_log.debug() << "yValue = " << yValue << tempy << std::endl;
              db1 = false;
            }

          } else if (filetype == 's'){
            // SLOG
            if (calslogx0){
              // calculation of x0 must use the x'[0]
              g_log.debug() << "x'_0 = " << xValue << "  bc3 = " << bc3 << std::endl;

              double x0 = 2*xValue/(bc3+2.0);
              X->push_back(x0);
              xPrev = x0;
              g_log.debug() << "SLOG: x0 = " << x0 << std::endl;
              calslogx0 = false;
            }

        	  xValue = (2 * xValue) - xPrev;
        	  if (multiplybybinwidth){
        	    yValue = yValue/(xValue-xPrev);
        	    eValue = eValue/(xValue-xPrev);
        	  } else {
        	    yValue = yValue;
        	    eValue = eValue;
        	  }
          }
          X->push_back(xValue);
          Y->push_back(yValue);
          E->push_back(eValue);
        }
      }
      if ( X->size() != 0 )
      { // Put final spectra into data
        gsasDataX.push_back(X);
        gsasDataY.push_back(Y);
        gsasDataE.push_back(E);
      }
      input.close();
    }

    int nHist(static_cast<int>(gsasDataX.size()));
    int xWidth(static_cast<int>(X->size()));
    int yWidth(static_cast<int>(Y->size()));

    // Create workspace
    MatrixWorkspace_sptr outputWorkspace = boost::dynamic_pointer_cast<MatrixWorkspace> (WorkspaceFactory::Instance().create("Workspace2D", nHist, xWidth, yWidth));
    // GSS Files data is always in TOF
    outputWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    // Set workspace title
    if (filetype == 'r'){
      outputWorkspace->setTitle(wsTitle);
    } else {
      outputWorkspace->setTitle(slogTitle);
    }
    // Put data from MatidVec's into outputWorkspace
    for ( int i = 0; i < nHist; ++i )
    {
      // Move data across
      outputWorkspace->dataX(i) = *gsasDataX[i];
      outputWorkspace->dataY(i) = *gsasDataY[i];
      outputWorkspace->dataE(i) = *gsasDataE[i];
      // Clean up after copy
      delete gsasDataX[i];
      delete gsasDataY[i];
      delete gsasDataE[i];
    }

    // Clean up
    delete prog;

    setProperty("OutputWorkspace", outputWorkspace);
    return;
  }


  /**This method does a quick file type check by checking the first 100 bytes of the file
   *  @param filePath- path of the file including name.
   *  @param nread :: no.of bytes read
   *  @param header :: The first 100 bytes of the file as a union
   *  @return true if the given file is of type which can be loaded by this algorithm
   */
      bool LoadGSS::quickFileCheck(const std::string& filePath,size_t nread,const file_header& header)
      {
        // check the file extension
        std::string extn=extension(filePath);
        bool bascii;
        if (extn.compare("gsa"))
          bascii = true;
        else if (extn.compare("txt"))
          bascii = true;
        else
          bascii = false;

        // check the bit of header
        bool is_ascii (true);
        for(size_t i=0; i<nread; i++)
        {
          if (!isascii(header.full_hdr[i]))
            is_ascii =false;
        }
        return(is_ascii|| bascii);
      }

  /**checks the file by opening it and reading few lines
   *  @param filePath :: name of the file including its path
   *  @return an integer value how much this algorithm can load the file
   */
      int LoadGSS::fileCheck(const std::string& filePath)
      {
        std::ifstream file(filePath.c_str());
        if (!file)
        {
          g_log.error("Unable to open file: " + filePath);
          throw Exception::FileError("Unable to open file: " , filePath);
        }
        std::string str;
        getline(file,str);//workspace title first line
        while (!file.eof())
        {
          getline(file,str);
          if(str.empty()||str[0]=='#')
          {
            continue;
          }
          if(!str.substr(0,4).compare("BANK")&& (str.find("RALF")!=std::string::npos || str.find("SLOG")!=std::string::npos)&& (str.find("FXYE")!=std::string::npos))
          {
            return 80;
          }
          return 0;
        }
        return  0;

      }

}//namespace
}//namespace
