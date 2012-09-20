/* WIKI*
dfkl;aflfk;

*WIKI*/
#include "MantidMDAlgorithms/PreprocessDetectorsToMD.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidAPI/NumericAxis.h"
//#include "


using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;


namespace Mantid
{
  namespace MDAlgorithms
  {
    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(PreprocessDetectorsToMD)

    PreprocessDetectorsToMD::PreprocessDetectorsToMD()
    {};

    // Sets documentation strings for this algorithm
    void PreprocessDetectorsToMD::initDocs()
    {    
      this->setWikiSummary("Servicing algorigthm to preprocess detector's positions namely  \n"
        "to perform generic part of the transformation from a matrix workspace of a real instrument to\n"
        "physical MD workspace of an experimental results (e.g Q-space).");
      this->setOptionalMessage("preprocess detector's positions namely perform generic part of the transformation \n"
        "from a physical space of a real instrument to\n"
        "physical MD workspace of an experimental results (e.g Q-space).");
    }
    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties. */
    void PreprocessDetectorsToMD::init()
    {
      auto ws_valid = boost::make_shared<Kernel::CompositeValidator>();
      // workspace needs instrument
      ws_valid->add<API::InstrumentValidator>();
      // the validator which checks if the workspace has axis and any units
      //ws_valid->add<API::WorkspaceUnitValidator>("");


      declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Kernel::Direction::Input,ws_valid),
        "An input Matrix Workspace");

      declareProperty(new WorkspaceProperty<TableWorkspace>("OutputWorkspace","",Kernel::Direction::Output),
        "Name of the output Table workspace. If the workspace exists, it will be replaced");

      //declareProperty(new PropertyWithValue<bool>("FakeDetectors",false,Kernel::Direction::Input),
      //  "If selected, generates table workspace with fake detectors, all allocated in a monitor position.\n"
      //  "Number of detectors is equal to number of spectra and real detectors, if present are ignored ");


    }

    //----------------------------------------------------------------------------------------------
    /* Execute the algorithm.   */
    void PreprocessDetectorsToMD::exec()
    {
      // -------- get Input workspace
      MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");  
      // -------- build target workspace:
      auto  targWS = createTableWorkspace(inputWS);
      if(this->isDetInfoLost(inputWS))
        this->buildFakeDetectorsPositions(inputWS,targWS);
      else    // process real detectors positions
        this->processDetectorsPositions(inputWS,targWS);

      // set up target workspace 
      setProperty("OutputWorkspace",targWS);
    }
    /** helper method to create resilting table workspace */
    boost::shared_ptr<DataObjects::TableWorkspace> PreprocessDetectorsToMD::createTableWorkspace(const API::MatrixWorkspace_const_sptr &inputWS)
    {
      const size_t nHist = inputWS->getNumberHistograms();

      // set the target workspace
      auto targWS = boost::shared_ptr<TableWorkspace>(new TableWorkspace(nHist));
      // detectors positions
      if(!targWS->addColumn("V3D","DetDirections"))throw(std::runtime_error("Can not add column DetDirectrions"));
      // sample-detector distance;
      if(!targWS->addColumn("double","L2"))throw(std::runtime_error("Can not add column L2"));
      // Diffraction angle
      if(!targWS->addColumn("double","TwoTheta"))throw(std::runtime_error("Can not add column TwoTheta"));
      if(!targWS->addColumn("double","Azimuthal"))throw(std::runtime_error("Can not add column Azimuthal"));
      // the detector ID;
      if(!targWS->addColumn("int32_t","DetectorID"))throw(std::runtime_error("Can not add column DetectorID"));
      // stores spectra index which corresponds to a valid detector index;
      if(!targWS->addColumn("size_t","detIDMap"))throw(std::runtime_error("Can not add column detIDMap"));
      // stores detector index which corresponds to the workspace index;
      if(!targWS->addColumn("size_t","spec2detMap"))throw(std::runtime_error("Can not add column spec2detMap"));
      // will see about that
      // sin^2(Theta)
      //    std::vector<double>      SinThetaSq;

      targWS->declareProperty(new Kernel::PropertyWithValue<std::string>("InstrumentName",""),"The name which should unique identify current instrument");
      targWS->declareProperty(new Kernel::PropertyWithValue<double>("L1",0),"L1 is the source to sample distance");
      targWS->declareProperty(new Kernel::PropertyWithValue<uint32_t>("ActualDetectorsNum",0),"The actual number of detectors receivinv signal");
      targWS->declareProperty(new Kernel::PropertyWithValue<bool>("FakeDetectors",false),"If the detectors were actually processed from real instrument or generated for some fake one ");
      return targWS;
    }

    /** method does preliminary calculations of the detectors positions to convert results into k-dE space ;
    and places the resutls into static cash to be used in subsequent calls to this algorithm */
    void PreprocessDetectorsToMD::processDetectorsPositions(const API::MatrixWorkspace_const_sptr &inputWS,DataObjects::TableWorkspace_sptr &targWS)
    {
      g_log.information()<<" Preprocessing detectors locations in a target reciprocal space\n";
      // 
      Geometry::Instrument_const_sptr instrument = inputWS->getInstrument();
      //this->pBaseInstr                = instrument->baseInstrument();
      //
      Geometry::IObjComponent_const_sptr source = instrument->getSource();
      Geometry::IObjComponent_const_sptr sample = instrument->getSample();
      if ((!source) || (!sample)) 
      {
        g_log.error()<<" Instrument is not fully defined. Can not identify source or sample\n";
        throw Kernel::Exception::InstrumentDefinitionError("Instrubment not sufficiently defined: failed to get source and/or sample");
      }

      // L1
      try
      {
        double L1  = source->getDistance(*sample);
        targWS->setProperty<double>("L1",L1);
        g_log.debug() << "Source-sample distance: " << L1 << std::endl;
      }
      catch (Kernel::Exception::NotFoundError &)
      { 
        throw Kernel::Exception::InstrumentDefinitionError("Unable to calculate source-sample distance for workspace", inputWS->getTitle());  
      }
      // Instrument name
      std::string InstrName=instrument->getName();
      targWS->setProperty<std::string>("InstrumentName",InstrName);
      targWS->setProperty<bool>("FakeDetectors",false);

      // get access to the workspace memory
      auto &sp2detMap  = targWS->getColVector<size_t>("spec2detMap");
      auto &detId      = targWS->getColVector<int32_t>("DetectorID");
      auto &detIDMap   = targWS->getColVector<size_t>("detIDMap");
      auto &L2         = targWS->getColVector<double>("L2");
      auto &TwoTheta   = targWS->getColVector<double>("TwoTheta");
      auto &Azimuthal  = targWS->getColVector<double>("Azimuthal");
      auto &detDir     = targWS->getColVector<Kernel::V3D>("DetDirections"); 

      //// progress messave appearence
      size_t div=100;
      size_t nHist = targWS->rowCount();
      Mantid::API::Progress theProgress(this,0,1,nHist);
      //// Loop over the spectra
      uint32_t liveDetectorsCount(0);
      for (size_t i = 0; i < nHist; i++)
      {
        sp2detMap[i]=std::numeric_limits<size_t>::quiet_NaN();
        detId[i]    =std::numeric_limits<int32_t>::quiet_NaN();
        detIDMap[i] =std::numeric_limits<size_t>::quiet_NaN();
        L2[i]       =std::numeric_limits<double>::quiet_NaN(); 
        TwoTheta[i] =std::numeric_limits<double>::quiet_NaN(); 
        Azimuthal[i]=std::numeric_limits<double>::quiet_NaN(); 

        // get detector or detector group which corresponds to the spectra i
        Geometry::IDetector_const_sptr spDet;
        try
        {
          spDet= inputWS->getDetector(i);      
        }
        catch(Kernel::Exception::NotFoundError &)
        {
          continue;
        }

        // Check that we aren't dealing with monitor...
        if (spDet->isMonitor())continue;   

        // calculate the requested values;
        sp2detMap[i]                = liveDetectorsCount;
        detId[liveDetectorsCount]   = int32_t(spDet->getID());
        detIDMap[liveDetectorsCount]= i;
        L2[liveDetectorsCount]      = spDet->getDistance(*sample);

        double polar   =  inputWS->detectorTwoTheta(spDet);
        double azim    =  spDet->getPhi();    
        TwoTheta[liveDetectorsCount]  =  polar;
        Azimuthal[liveDetectorsCount] =  azim;

        double sPhi=sin(polar);
        double ez = cos(polar);
        double ex = sPhi*cos(azim);
        double ey = sPhi*sin(azim);

        detDir[liveDetectorsCount].setX(ex);
        detDir[liveDetectorsCount].setY(ey);
        detDir[liveDetectorsCount].setZ(ez);

        //double sinTheta=sin(0.5*polar);
        //this->SinThetaSq[liveDetectorsCount]  = sinTheta*sinTheta;

        liveDetectorsCount++;
        if(i%div==0) theProgress.report(i,"Preprocessing detectors");

      }
      targWS->setProperty<uint32_t>("ActualDetectorsNum",liveDetectorsCount);

      theProgress.report();
      g_log.information()<<"finished preprocessing detectors locations, found: "<<liveDetectorsCount<<" detectors out of: "<<nHist<<" Histohrams\n";
    }

    /** method calculates fake detectors positions in the situation when real detector information has been lost  */
    void PreprocessDetectorsToMD::buildFakeDetectorsPositions(const API::MatrixWorkspace_const_sptr &inputWS,DataObjects::TableWorkspace_sptr &targWS)
    {
      UNUSED_ARG(inputWS);
      // set sample-detector postion equal to 1;
      targWS->setProperty<double>("L1",1.);
      // 
      targWS->setProperty<std::string>("InstrumentName","FakeInstrument");    
      targWS->setProperty<bool>("FakeDetectors",true);

      // get access to the workspace memory
      auto &sp2detMap  = targWS->getColVector<size_t>("spec2detMap");
      auto &detId      = targWS->getColVector<int32_t>("DetectorID");
      auto &detIDMap   = targWS->getColVector<size_t>("detIDMap");
      auto &L2         = targWS->getColVector<double>("L2");
      auto &TwoTheta   = targWS->getColVector<double>("TwoTheta");
      auto &Azimuthal  = targWS->getColVector<double>("Azimuthal");
      auto &detDir     = targWS->getColVector<Kernel::V3D>("DetDirections"); 


      //// progress messave appearence  
      size_t nHist = targWS->rowCount();
      targWS->setProperty<uint32_t>("ActualDetectorsNum",uint32_t(nHist));

      double polar(0);
      // Loop over the spectra
      for (size_t i = 0; i < nHist; i++)
      {
        sp2detMap[i]= i;
        detId[i]    = (detid_t)i;
        detIDMap[i] = i;
        L2[i]       = 1;

        TwoTheta[i] =  polar;
        Azimuthal[i] = 0;
        //this->SinThetaSq[i]= 0;

        double ez = 1.;
        double ex = 0.;
        double ey = 0.;

        detDir[i].setX(ex);
        detDir[i].setY(ey);
        detDir[i].setZ(ez);

      }
      // 
    }

    /// function checks if source workspace still has information about detectors. Some ws (like rebinned one) do not have this information any more. 
    bool PreprocessDetectorsToMD::isDetInfoLost(Mantid::API::MatrixWorkspace_const_sptr inWS2D)const
    {
      auto pYAxis = dynamic_cast<API::NumericAxis *>(inWS2D->getAxis(1));
      // if this is numeric axis, then the detector's information has been lost:
      if(pYAxis)     return true; 
      return false;
    }


  }
}