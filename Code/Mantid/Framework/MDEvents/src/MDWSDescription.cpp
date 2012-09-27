#include "MantidMDEvents/MDWSDescription.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidMDEvents/MDTransfFactory.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/Strings.h"

#include <boost/lexical_cast.hpp>


namespace Mantid
{
namespace MDEvents
{

/** set specific (non-default) dimension name 
* @param nDim   -- number of dimension;
* @param Name   -- the name to assighn into diemnsion names vector;
*/
void MDWSDescription::setDimName(unsigned int nDim,const std::string &Name)
{
  if(nDim>=m_NDims)
  {
    std::string ERR = "setDimName::Dimension index: "+boost::lexical_cast<std::string>(nDim)+" out of total dimensions range: "+boost::lexical_cast<std::string>(m_NDims);
    throw(std::invalid_argument(ERR));
  }
  m_DimNames[nDim] = Name;
}
/** this is rather misleading function, as MD workspace does not currently have dimension units. 
*It actually sets the units dimension names, which will be displayed along axis and have nothinbg in common with units, defined by unit factory */
void MDWSDescription::setDimUnit(unsigned int nDim,const std::string &Unit)
{
  if(nDim>=m_NDims)
  {
    std::string ERR = "setDimUnit::Dimension index: "+boost::lexical_cast<std::string>(nDim)+" out of total dimensions range: "+boost::lexical_cast<std::string>(m_NDims);
    throw(std::invalid_argument(ERR));
  }
  m_DimUnits[nDim] = Unit;
}

/** the method builds the MD ws description from existing matrix workspace and the requested transformation parameters. 
*@param  pWS    -- input matrix workspace to be converted into MD workpsace
*@param  QMode  -- momentum conversion mode. Any mode supported by Q conversion factory. Class just carries up the name of Q-mode, 
*                  to the place where factory call to the solver is made , so no code modification is needed when new modes are added 
*                  to the factory
*@param  dEMode  -- energy analysis mode (string representation). Should correspond to energy analysis modes, supported by selected Q-mode
*@param  dimPropertyNames -- the vector of names for additional ws properties, which will be used as dimensions.

*/
void MDWSDescription::buildFromMatrixWS(const API::MatrixWorkspace_const_sptr &pWS,const std::string &QMode,const std::string dEMode,
  const std::vector<std::string> &dimProperyNames)
{
  m_InWS = pWS;
  // fill additional dimensions values, defined by workspace properties;
  this->fillAddProperties(m_InWS,dimProperyNames,m_AddCoord);

  this->AlgID = QMode;

  // check and get energy conversion mode;
  MDTransfDEHelper dEChecker;
  m_Emode = dEChecker.getEmode(dEMode);

  // get raw pointer to Q-transformation (do not delete this pointer!)
  MDTransfInterface* pQtransf =  MDTransfFactory::Instance().create(QMode).get();

  // get number of dimensions this Q transformation generates from the workspace. 
  unsigned int nMatrixDim = pQtransf->getNMatrixDimensions(m_Emode,m_InWS);

  // number of MD ws dimensions is the sum of n-matrix dimensions and dimensions coming from additional coordinates
  m_NDims  = nMatrixDim + (unsigned int)m_AddCoord.size();
  this->resizeDimDescriptions(m_NDims);
  // check if all MD dimensions descriptors are set properly
  if(m_NDims!=m_DimNames.size()||m_NDims!=m_DimMin.size())
    throw(std::invalid_argument(" dimension limits vectors and dimension description vectors inconsistent as have different length"));


  //*********** fill in dimension id-s, dimension units and dimension names
  // get default dim ID-s. TODO: it should be possibility to owerride it later;
  std::vector<std::string> MatrDimID   = pQtransf->getDefaultDimID(m_Emode,m_InWS);
  std::vector<std::string> MatrUnitID  = pQtransf->outputUnitID(m_Emode,m_InWS);
  for(unsigned int i=0;i<m_NDims;i++)
  {
    if(i<nMatrixDim)
    {
      m_DimIDs[i]  = MatrDimID[i];
      m_DimNames[i]= MatrDimID[i];
      m_DimUnits[i]= MatrUnitID[i];
    }
    else
    {
      m_DimIDs[i]  = dimProperyNames[i-nMatrixDim];
      m_DimNames[i]= dimProperyNames[i-nMatrixDim];
      m_DimUnits[i]= dimProperyNames[i-nMatrixDim];
    }
  }


  //Set up goniometer. Empty ws's goniometer returns unit transformation matrix
  m_GoniomMatr = m_InWS->run().getGoniometer().getR();

} 


/** the function builds MD event WS description from existing workspace. 
* Primary used to obtain existing ws parameters 
*@param pWS -- shared pointer to existing MD workspace 
*/
void MDWSDescription::buildFromMDWS(const API::IMDEventWorkspace_const_sptr &pWS)
{
  m_NDims = (unsigned int)pWS->getNumDims();
  // prepare all arrays:
  m_DimNames.resize(m_NDims);
  m_DimIDs.resize(m_NDims);
  m_DimUnits.resize(m_NDims);   

  m_NBins.resize(m_NDims);
  m_DimMin.resize(m_NDims);
  m_DimMax.resize(m_NDims);
  for(size_t i=0;i<m_NDims;i++)
  {
    const Geometry::IMDDimension *pDim = pWS->getDimension(i).get();
    m_DimNames[i]= pDim->getName();
    m_DimIDs[i]  = pDim->getDimensionId();
    m_DimUnits[i]= pDim->getUnits();   

    m_NBins[i]   = pDim->getNBins();
    m_DimMin[i]  = pDim->getMinimum();
    m_DimMax[i]  = pDim->getMaximum();

  }
  m_Wtransf = Kernel::DblMatrix(pWS->getWTransf()); 


}
/** When the workspace has been build from existing MDWrokspace, some target worskpace parameters can not be defined,
* as these parameters are defined by the algorithm and input matrix workspace.
*  examples are emode or input energy, which is actually source workspace parameters, or some other parameters 
*  defined by the transformation algorithm
* 
* This method used to define such parameters from MDWS description, build from workspace and the transformation algorithm parameters
*
*@param SourceMartWS -- the MDWS description obtained from input matrix workspace and the algorithm parameters
*/
void MDWSDescription::setUpMissingParameters(const MDEvents::MDWSDescription &SourceMatrWS)
{
  m_InWS  = SourceMatrWS.m_InWS;
  m_Emode = SourceMatrWS.m_Emode;
  this->AlgID = SourceMatrWS.AlgID;

  m_AddCoord.assign(SourceMatrWS.m_AddCoord.begin(),SourceMatrWS.m_AddCoord.end());

  m_GoniomMatr = SourceMatrWS.m_GoniomMatr;

}


/**function compares old workspace description with the new workspace description, defined by the algorithm properties and 
* selects/changes the properties which can be changed through input parameters given that target MD workspace exist   
*
* This situation occurs if the base description has been obtained from MD workspace, and one is building a description from 
* other matrix workspace to add new data to the existing workspace. The workspaces have to be comparable.
*
* @param NewMDWorkspaceD -- MD workspace description, obtained from algorithm parameters
*
* @returns NewMDWorkspaceD -- modified md workspace description, which is compartible with existing MD workspace
*
*/
void  MDWSDescription::checkWSCorresponsMDWorkspace(MDEvents::MDWSDescription &NewMDWorkspaceD)
{
  if(m_NDims != NewMDWorkspaceD.m_NDims)
  {
    std::string ERR = "Dimension numebrs are inconsistent: this workspace has "+boost::lexical_cast<std::string>(m_NDims)+" dimensions and target one: "+
                       boost::lexical_cast<std::string>(NewMDWorkspaceD.m_NDims);
    throw(std::invalid_argument(ERR)); 
  }

  if(m_Emode==CnvrtToMD::Undef)
    throw(std::invalid_argument("Workspace description has not been correctly defined, as emode has not been defined")); 

  //TODO: More thorough checks may be nesessary to prevent adding different kind of workspaces e.g 4D |Q|-dE-T-P workspace to Q3d+dE ws
}

/// empty constructor
MDWSDescription::MDWSDescription(unsigned int nDimensions):
  m_Wtransf(3,3,true),
  m_GoniomMatr(3,3,true),
  m_RotMatrix(9,0),       // set transformation matrix to 0 to certainly see rubbish if error later
  m_Emode(CnvrtToMD::Undef),
  m_LorentzCorr(false)
{

  this->resizeDimDescriptions(nDimensions);
  m_DimMin.assign(m_NDims,std::numeric_limits<double>::quiet_NaN());
  m_DimMax.assign(m_NDims,std::numeric_limits<double>::quiet_NaN());

}
void MDWSDescription::resizeDimDescriptions(unsigned int nDimensions, size_t nBins)
{
  m_NDims = nDimensions;

  m_DimNames.assign(m_NDims,"mdn");
  m_DimIDs.assign(m_NDims,"mdn_");
  m_DimUnits.assign(m_NDims,"Momentum");
  m_NBins.assign(m_NDims,nBins);

  for(size_t i=0;i<m_NDims;i++)
  {
    m_DimIDs[i]  = m_DimIDs[i]+boost::lexical_cast<std::string>(i);
    m_DimNames[i]= m_DimNames[i]+boost::lexical_cast<std::string>(i);
  }

}
/**function sets up min-max values to the dimensions, described by the class
*@param minVal  -- vector of minimal dimension's values
*@param maxVal  -- vector of maximal dimension's values
* 
*/
void MDWSDescription::setMinMax(const std::vector<double> &minVal,const std::vector<double> &maxVal)
{
  m_DimMin.assign(minVal.begin(),minVal.end());
  m_DimMax.assign(maxVal.begin(),maxVal.end());

  this->checkMinMaxNdimConsistent(m_DimMin,m_DimMax);
}

/**get vector of minimal and maximal values from the class */
void MDWSDescription::getMinMax(std::vector<double> &min,std::vector<double> &max)const
{
  min.assign(m_DimMin.begin(),m_DimMin.end());
  max.assign(m_DimMax.begin(),m_DimMax.end());
}
//******************************************************************************************************************************************
//*************   HELPER FUNCTIONS
//******************************************************************************************************************************************
/** Returns symbolic representation of current Emode */
std::string MDWSDescription::getEModeStr()const
{
  MDTransfDEHelper         deHelper;
  return deHelper.getEmode(m_Emode);
}

/** function extracts the coordinates from additional workspace porperties and places them to proper position within 
*  the vector of MD coodinates for the particular workspace.
*
*  @param dimProperyNames  -- names of properties which should be treated as dimensions
*
*  @returns AddCoord       -- vector of additional coordinates (derived from WS properties) for current multidimensional event
*/
void MDWSDescription::fillAddProperties(Mantid::API::MatrixWorkspace_const_sptr inWS2D,const std::vector<std::string> &dimProperyNames,std::vector<coord_t> &AddCoord)
{
  size_t nDimPropNames = dimProperyNames.size();
  if(AddCoord.size()!=nDimPropNames)AddCoord.resize(nDimPropNames);

  for(size_t i=0;i<nDimPropNames;i++)
  {
    //HACK: A METHOD, Which converts TSP into value, correspondent to time scale of matrix workspace has to be developed and deployed!
    Kernel::Property *pProperty = (inWS2D->run().getProperty(dimProperyNames[i]));
    Kernel::TimeSeriesProperty<double> *run_property = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(pProperty);  
    if(run_property)
    {
      AddCoord[i]=coord_t(run_property->firstValue());
    }
    else
    {
      // e.g Ei can be a property and dimenson
      Kernel::PropertyWithValue<double> *proc_property = dynamic_cast<Kernel::PropertyWithValue<double> *>(pProperty);  
      if(!proc_property)
      {
        std::string ERR = " Can not interpret property, used as dimension.\n Property: "+dimProperyNames[i]+
          " is neither a time series (run) property nor a property with value<double>";
        throw(std::invalid_argument(ERR));
      }
      AddCoord[i]  = coord_t(*(proc_property));
    }
  }
}

/** function verifies the consistency of the min and max dimsnsions values  checking if all necessary 
* values vere defined and min values are smaller then mav values */
void MDWSDescription::checkMinMaxNdimConsistent(const std::vector<double> &minVal,const std::vector<double> &maxVal)
{
  if(minVal.size()!=maxVal.size())
  {
    std::string ERR = " number of specified min dimension values: "+boost::lexical_cast<std::string>(minVal.size())+
                      " and number of max values: "+boost::lexical_cast<std::string>(maxVal.size())+
                      " are not consistent\n";
    throw(std::invalid_argument(ERR));
  }

  for(size_t i=0; i<minVal.size();i++)
  {
    if(maxVal[i]<=minVal[i])
    {
      std::string ERR = " min value "+boost::lexical_cast<std::string>(minVal[i])+
                        " not less then max value"+boost::lexical_cast<std::string>(maxVal[i])+" in direction: "+
                        boost::lexical_cast<std::string>(i)+"\n";
      throw(std::invalid_argument(ERR));
    }
  }
}

 /** function retrieves copy of the oriented lattice from the workspace */
boost::shared_ptr<Geometry::OrientedLattice> MDWSDescription::getOrientedLattice(Mantid::API::MatrixWorkspace_const_sptr inWS2D)
{
  // try to get the WS oriented lattice
  boost::shared_ptr<Geometry::OrientedLattice> orl;
  if(inWS2D->sample().hasOrientedLattice())
    orl = boost::shared_ptr<Geometry::OrientedLattice>(new Geometry::OrientedLattice(inWS2D->sample().getOrientedLattice()));      
  
  return orl;

}


} //end namespace MDEvents 
} //end namespace Mantid
