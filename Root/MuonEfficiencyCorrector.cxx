/*************************************************
 *
 * Interface to CP Muon Efficiency Correction Tool.
 *
 * M. Milesi (marco.milesi@cern.ch)
 *
 *
 ************************************************/

// c++ include(s):
#include <iostream>

// EL include(s):
#include <EventLoop/Job.h>
#include <EventLoop/StatusCode.h>
#include <EventLoop/Worker.h>

// EDM include(s):
#include "xAODEventInfo/EventInfo.h"
#include "xAODMuon/MuonContainer.h"
#include "xAODMuon/Muon.h"
#include "xAODBase/IParticleHelpers.h"
#include "xAODBase/IParticleContainer.h"
#include "xAODBase/IParticle.h"
#include "AthContainers/ConstDataVector.h"
#include "AthContainers/DataVector.h"

// package include(s):
#include "xAODAnaHelpers/HelperFunctions.h"
#include "xAODAnaHelpers/HelperClasses.h"
#include "xAODAnaHelpers/MuonEfficiencyCorrector.h"

#include <xAODAnaHelpers/tools/ReturnCheck.h>

// ROOT include(s):
#include "TEnv.h"
#include "TSystem.h"

using HelperClasses::ToolName;

// this is needed to distribute the algorithm to the workers
ClassImp(MuonEfficiencyCorrector)


MuonEfficiencyCorrector :: MuonEfficiencyCorrector () :
  m_MuonEffSFTool(nullptr)
{
  // Here you put any code for the base initialization of variables,
  // e.g. initialize all pointers to 0.  Note that you should only put
  // the most basic initialization here, since this method will be
  // called on both the submission and the worker node.  Most of your
  // initialization code will go into histInitialize() and
  // initialize().

  Info("MuonEfficiencyCorrector()", "Calling constructor");

  // Read debug flag from .config file
  m_debug                      = false;

  // Input container to be read from TEvent or TStore
  m_inContainerName            = "";

  // Efficiency SF
  m_WorkingPoint               = "Loose";
  m_DataPeriod                 = "2015";

  // Trigger SF
  m_year                       = 2015;
  m_runNumber                  = 900000;
  m_SingleMuTrig               = "HLT_mu20_iloose_L1MU15";
  m_SinglePlusDiMuTrig         = "mu20_iloose_L1MU15_or_mu50_or_2mu14";

  // Systematics stuff
  m_inputAlgoSystNames         = "";
  m_systNameEff		           = "";
  m_systNameTrig		       = "";
  m_outputSystNamesEff         = "MuonEfficiencyCorrector_EffSyst";
  m_outputSystNamesTrig        = "MuonEfficiencyCorrector_TrigSyst";
  m_systValEff 		           = 0.;
  m_systValTrig 		       = 0.;

}


EL::StatusCode  MuonEfficiencyCorrector :: configure ()
{

  if ( !getConfig().empty() ) {

    Info("configure()", "Configuing MuonEfficiencyCorrector Interface. User configuration read from : %s ", getConfig().c_str());

    TEnv* config = new TEnv(getConfig(true).c_str());

    // Read debug flag from .config file
    m_debug                      = config->GetValue("Debug", m_debug);

    // Input container to be read from TEvent or TStore
    m_inContainerName            = config->GetValue("InputContainer",  m_inContainerName.c_str());

    // Efficiency SF
    m_WorkingPoint               = config->GetValue("WorkingPoint", m_WorkingPoint.c_str());
    m_DataPeriod                 = config->GetValue("DataPeriod",   m_DataPeriod.c_str());

    // Trigger SF
    m_year                       = config->GetValue("year", m_year);
    m_runNumber                  = config->GetValue("runNumber", m_runNumber);
    m_SingleMuTrig               = config->GetValue("SingleMuTrig", m_SingleMuTrig.c_str());
    m_SinglePlusDiMuTrig         = config->GetValue("SinglePlusDiMuTrig", m_SinglePlusDiMuTrig.c_str());

    // Systematics stuff
    m_inputAlgoSystNames         = config->GetValue("InputAlgoSystNames",  m_inputAlgoSystNames.c_str());
    m_systNameEff		 = config->GetValue("SystNameEff" , m_systNameEff.c_str());
    m_systNameTrig		 = config->GetValue("SystNameTrig" , m_systNameTrig.c_str());
    m_outputSystNamesEff         = config->GetValue("OutputSystNamesEff",  m_outputSystNamesEff.c_str());
    m_outputSystNamesTrig        = config->GetValue("OutputSystNamesTrig", m_outputSystNamesTrig.c_str());
    m_systValEff 		 = config->GetValue("SystValEff" , m_systValEff);
    m_systValTrig 		 = config->GetValue("SystValTrig" , m_systValTrig);
    m_runAllSystEff              = (m_systNameEff.find("All") != std::string::npos);
    m_runAllSystTrig             = (m_systNameTrig.find("All") != std::string::npos);

    config->Print();

    Info("configure()", "ElectronEfficiencyCorrector Interface succesfully configured! ");

    delete config; config = nullptr;
  }

  if ( m_inContainerName.empty() ) {
    Error("configure()", "InputContainer is empty!");
    return EL::StatusCode::FAILURE;
  }

  return EL::StatusCode::SUCCESS;
}


EL::StatusCode MuonEfficiencyCorrector :: setupJob (EL::Job& job)
{
  // Here you put code that sets up the job on the submission object
  // so that it is ready to work with your algorithm, e.g. you can
  // request the D3PDReader service or add output files.  Any code you
  // put here could instead also go into the submission script.  The
  // sole advantage of putting it here is that it gets automatically
  // activated/deactivated when you add/remove the algorithm from your
  // job, which may or may not be of value to you.

  Info("setupJob()", "Calling setupJob");

  job.useXAOD ();
  xAOD::Init( "MuonEfficiencyCorrector" ).ignore(); // call before opening first file

  return EL::StatusCode::SUCCESS;
}



EL::StatusCode MuonEfficiencyCorrector :: histInitialize ()
{
  // Here you do everything that needs to be done at the very
  // beginning on each worker node, e.g. create histograms and output
  // trees.  This method gets called before any input files are
  // connected.
  return EL::StatusCode::SUCCESS;
}



EL::StatusCode MuonEfficiencyCorrector :: fileExecute ()
{
  // Here you do everything that needs to be done exactly once for every
  // single file, e.g. collect a list of all lumi-blocks processed
  return EL::StatusCode::SUCCESS;
}



EL::StatusCode MuonEfficiencyCorrector :: changeInput (bool /*firstFile*/)
{
  // Here you do everything you need to do when we change input files,
  // e.g. resetting branch addresses on trees.  If you are using
  // D3PDReader or a similar service this method is not needed.
  return EL::StatusCode::SUCCESS;
}



EL::StatusCode MuonEfficiencyCorrector :: initialize ()
{
  // Here you do everything that you need to do after the first input
  // file has been connected and before the first event is processed,
  // e.g. create additional histograms based on which variables are
  // available in the input files.  You can also create all of your
  // histograms and trees in here, but be aware that this method
  // doesn't get called if no events are processed.  So any objects
  // you create here won't be available in the output if you have no
  // input events.

  Info("initialize()", "Initializing MuonEfficiencyCorrector Interface... ");

  m_event = wk()->xaodEvent();
  m_store = wk()->xaodStore();

  Info("initialize()", "Number of events in file: %lld ", m_event->getEntries() );

  if ( this->configure() == EL::StatusCode::FAILURE ) {
    Error("initialize()", "Failed to properly configure. Exiting." );
    return EL::StatusCode::FAILURE;
  }

  m_numEvent      = 0;
  m_numObject     = 0;

  // initialize the Muon Efficiency Correction Tool
  std::string eff_tool_name = std::string("MuonEfficiencyScaleFactors_") + m_name;
  m_MuonEffSFTool = new CP::MuonEfficiencyScaleFactors( eff_tool_name.c_str() );
  m_MuonEffSFTool->msg().setLevel( MSG::INFO ); // DEBUG, VERBOSE, INFO, ERROR

  RETURN_CHECK( "MuonEfficiencyCorrector::initialize()", m_MuonEffSFTool->setProperty("WorkingPoint", m_WorkingPoint ),"Failed to set Working Point property of MuonEfficiencyScaleFactors");
  RETURN_CHECK( "MuonEfficiencyCorrector::initialize()", m_MuonEffSFTool->setProperty("DataPeriod", m_DataPeriod ),"Failed to set DataPeriod property of MuonEfficiencyScaleFactors");
  // test audit trail
  RETURN_CHECK( "MuonEfficiencyCorrector::initialize()", m_MuonEffSFTool->setProperty("doAudit", false),"Failed to set doAudit property of MuonEfficiencyScaleFactors");
  RETURN_CHECK( "MuonEfficiencyCorrector::initialize()", m_MuonEffSFTool->initialize(), "Failed to properly initialize MuonEfficiencyScaleFactors");

  // Get a list of affecting systematics
  if ( m_debug ) {
    CP::SystematicSet affectSystsEff = m_MuonEffSFTool->affectingSystematics();
    // Convert into a simple list
    std::vector<CP::SystematicSet> affectSystEffList = CP::make_systematics_vector(affectSystsEff);
    for ( const auto& syst_it : affectSystsEff ) { Info("initialize()"," MuonEfficiencyScaleFactors tool can be affected by systematic: %s", (syst_it.name()).c_str()); }
  }

  // Get a list of recommended systematics
  const CP::SystematicSet recSystsEff = m_MuonEffSFTool->recommendedSystematics();
  m_systListEff = HelperFunctions::getListofSystematics( recSystsEff, m_systNameEff, m_systValEff );
  // Convert into a simple list
  m_systListEff = CP::make_systematics_vector(recSystsEff);
  for ( const auto& syst_it : m_systListEff ) {
      Info("initialize()","MuonEfficiencyScaleFactors tool recommended systematic: %s", (syst_it.name()).c_str());
  }

  if ( m_systNameEff.empty() ) {
      Info("initialize()","MuonEfficiencyScaleFactors tool - Running w/ nominal configuration!");
  }

  // Initialise the Muon Trigger Scale Factor tool
  std::string trig_tool_name = std::string("MuonTriggerScaleFactors_") + m_name;
  m_MuonTrigSFTool = new CP::MuonTriggerScaleFactors( trig_tool_name.c_str() );
  m_MuonTrigSFTool->msg().setLevel( MSG::INFO ); // DEBUG, VERBOSE, INFO, ERROR

  RETURN_CHECK( "MuonEfficiencyCorrector::initialize()", m_MuonTrigSFTool->setProperty("year", m_year ),"Failed to set year property of MuonTriggerScaleFactors");
  RETURN_CHECK( "MuonEfficiencyCorrector::initialize()", m_MuonTrigSFTool->setProperty("runNumber", m_runNumber ),"Failed to set runNumber property of MuonTriggerScaleFactors");
  int sys(0); // 0 : Combined, 1 : SysOnly, 2 : StatOnly
  RETURN_CHECK( "MuonEfficiencyCorrector::initialize()", m_MuonTrigSFTool->setProperty("SystematicOption", sys ),"Failed to set SystematicOption property of MuonTriggerScaleFactors");

  RETURN_CHECK( "MuonEfficiencyCorrector::initialize()", m_MuonTrigSFTool->initialize(), "Failed to properly initialize MuonTriggerScaleFactors");

  // Get a list of affecting systematics
  if ( m_debug ) {
    CP::SystematicSet affectSystsTrig = m_MuonTrigSFTool->affectingSystematics();
    // Convert into a simple list
    std::vector<CP::SystematicSet> affectSystTrigList = CP::make_systematics_vector(affectSystsTrig);
    for ( const auto& syst_it : affectSystTrigList ) { Info("initialize()"," MuonEfficiencyScaleFactors tool can be affected by systematic: %s", (syst_it.name()).c_str()); }
  }

  // Get a list of recommended systematics
  CP::SystematicSet recSystsTrig = m_MuonTrigSFTool->recommendedSystematics();
  m_systListTrig = HelperFunctions::getListofSystematics( recSystsTrig, m_systNameTrig, m_systValTrig );
  // Convert into a simple list
  m_systListTrig = CP::make_systematics_vector(recSystsTrig);
  for ( const auto& syst_it : m_systListTrig ) {
      Info("initialize()","MuonTriggerScaleFactors tool recommended systematic: %s", (syst_it.name()).c_str());
  }

  if ( m_systNameTrig.empty() ) {
      Info("initialize()","MuonTriggerScaleFactors tool - Running w/ nominal configuration!");
  }

  Info("initialize()", "MuonEfficiencyCorrector Interface succesfully initialized!" );

  return EL::StatusCode::SUCCESS;
}


EL::StatusCode MuonEfficiencyCorrector :: execute ()
{
  // Here you do everything that needs to be done on every single
  // events, e.g. read input variables, apply cuts, and fill
  // histograms and trees.  This is where most of your actual analysis
  // code will go.

  if ( m_debug ) { Info("execute()", "Applying Muon Efficiency and Trigger Correction... "); }

  m_numEvent++;

  const xAOD::EventInfo* eventInfo(nullptr);
  RETURN_CHECK("MuonEfficiencyCorrector::execute()", HelperFunctions::retrieve(eventInfo, m_eventInfoContainerName, m_event, m_store, m_debug) ,"");
  bool isMC = ( eventInfo->eventType( xAOD::EventInfo::IS_SIMULATION ) );
  if ( !isMC ) {
    if ( m_debug ) { Info("execute()", "Event is Data! Do not apply Muon Efficiency and Trigger Correction... "); }
    return EL::StatusCode::SUCCESS;
  }

  // initialise containers
  const xAOD::MuonContainer* inputMuons(nullptr);

  // if m_inputAlgoSystNames = "" --> input comes from xAOD, or just running one collection,
  // then get the one collection and be done with it

  // Declare a counter, initialised to 0
  // For the systematically varied input containers, we won't store again the vector with efficiency systs in TStore ( it will be always the same!)
  unsigned int countInputCont(0);

  if ( m_inputAlgoSystNames.empty() ) {

    RETURN_CHECK("MuonEfficiencyCorrector::execute()", HelperFunctions::retrieve(inputMuons, m_inContainerName, m_event, m_store, m_debug) ,"");

	// decorate muons w/ SF - there will be a decoration w/ different name for each syst!
    this->executeSF( inputMuons, eventInfo, countInputCont );

  } else {
  // if m_inputAlgo = NOT EMPTY --> you are retrieving syst varied containers from an upstream algo. This is the case of calibrators: one different SC
  // for each calibration syst applied

	// get vector of string giving the syst names of the upstream algo m_inputAlgo (rememeber: 1st element is a blank string: nominal case!)
        std::vector<std::string>* systNames(nullptr);
        RETURN_CHECK("MuonEfficiencyCorrector::execute()", HelperFunctions::retrieve(systNames, m_inputAlgoSystNames, 0, m_store, m_debug) ,"");

    	// loop over systematic sets available
    	for ( auto systName : *systNames ) {

           RETURN_CHECK("MuonEfficiencyCorrector::execute()", HelperFunctions::retrieve(inputMuons, m_inContainerName+systName, m_event, m_store, m_debug) ,"");

    	   if ( m_debug ){
    	     unsigned int idx(0);
    	     for ( auto mu : *(inputMuons) ) {
    	       Info( "execute", "Input muon %i, pt = %.2f GeV ", idx, (mu->pt() * 1e-3) );
    	       ++idx;
    	     }
    	   }

	   // decorate muons w/ SF - there will be a decoration w/ different name for each syst!
	   this->executeSF( inputMuons, eventInfo, countInputCont );

	   // increment counter
	   ++countInputCont;

    	} // close loop on systematic sets available from upstream algo

   }

  // look what do we have in TStore
  if ( m_debug ) { m_store->print(); }

  return EL::StatusCode::SUCCESS;

}


EL::StatusCode MuonEfficiencyCorrector :: postExecute ()
{
  // Here you do everything that needs to be done after the main event
  // processing.  This is typically very rare, particularly in user
  // code.  It is mainly used in implementing the NTupleSvc.

  if ( m_debug ) { Info("postExecute()", "Calling postExecute"); }

  return EL::StatusCode::SUCCESS;
}



EL::StatusCode MuonEfficiencyCorrector :: finalize ()
{
  // This method is the mirror image of initialize(), meaning it gets
  // called after the last event has been processed on the worker node
  // and allows you to finish up any objects you created in
  // initialize() before they are written to disk.  This is actually
  // fairly rare, since this happens separately for each worker node.
  // Most of the time you want to do your post-processing on the
  // submission node after all your histogram outputs have been
  // merged.  This is different from histFinalize() in that it only
  // gets called on worker nodes that processed input events.

  Info("finalize()", "Deleting tool instances...");

  if ( m_MuonEffSFTool ) { delete m_MuonEffSFTool; m_MuonEffSFTool = nullptr; }
  if ( m_MuonTrigSFTool ) { delete m_MuonTrigSFTool; m_MuonTrigSFTool = nullptr; }

  return EL::StatusCode::SUCCESS;
}


EL::StatusCode MuonEfficiencyCorrector :: histFinalize ()
{
  // This method is the mirror image of histInitialize(), meaning it
  // gets called after the last event has been processed on the worker
  // node and allows you to finish up any objects you created in
  // histInitialize() before they are written to disk.  This is
  // actually fairly rare, since this happens separately for each
  // worker node.  Most of the time you want to do your
  // post-processing on the submission node after all your histogram
  // outputs have been merged.  This is different from finalize() in
  // that it gets called on all worker nodes regardless of whether
  // they processed input events.

  Info("histFinalize()", "Calling histFinalize");

  return EL::StatusCode::SUCCESS;
}

EL::StatusCode MuonEfficiencyCorrector :: executeSF (  const xAOD::MuonContainer* inputMuons, const xAOD::EventInfo* eventInfo, unsigned int countSyst  )
{

  // decorate the muons with data driven reco efficiency
  for ( auto mu_itr : *(inputMuons) ) {
    if ( m_MuonEffSFTool->applyRecoEfficiency( *mu_itr ) != CP::CorrectionCode::Ok ) {
      Error( "execute()", "Problem in getEfficiencyScaleFactor");
      return EL::StatusCode::FAILURE;
    }
  }

  //
  // Every muon gets decorated with a pair of vector<double> of SFs ( efficiency SFs & trigger SFs ), one SF for each syst.
  // We create these vector<string> with the SF syst names, so that we know which component corresponds to.
  // These vectors are eventually stored in TStore
  //
  std::vector< std::string >* sysVariationNamesEff  = new std::vector< std::string >;
  std::vector< std::string >* sysVariationNamesTrig = new std::vector< std::string >;

  //
  // Efficiency SFs - this is a per-muon weight
  //
  // loop over available systematics for this tool - remember: syst == EMPTY_STRING --> nominal
  for ( const auto& syst_it : m_systListEff ) {

    //
    // Create the name of the SF weight to be recorded
    //   template:  SYSNAME_MuEff_SF
    //
    std::string sfName = "MuEff_SF";
    if ( !syst_it.name().empty() ) {
       std::string prepend = syst_it.name() + "_";
       sfName.insert( 0, prepend );
    }
    if ( m_debug ) { Info("execute()", "SF decoration name is: %s", sfName.c_str()); }
    sysVariationNamesEff->push_back(sfName);

    // apply syst
    if ( m_MuonEffSFTool->applySystematicVariation(syst_it) != CP::SystematicCode::Ok ) {
      Error("initialize()", "Failed to configure MuonEfficiencyScaleFactors for systematic %s", syst_it.name().c_str());
      return EL::StatusCode::FAILURE;
    }
    if ( m_debug ) { Info("execute()", "Successfully applied systematic: %s", syst_it.name().c_str()); }

    // and now apply efficiency SF!
    unsigned int idx(0);
    for ( auto mu_itr : *(inputMuons) ) {

       if ( m_debug ) { Info( "execute()", "Efficiency SF - Checking muon %i, pt = %.2f GeV ", idx, (mu_itr->pt() * 1e-3) ); }
       ++idx;

       //
       //  If SF decoration vector doesn't exist, create it (will be done only for the 1st systematic for *this* muon)
       //
       SG::AuxElement::Decorator< std::vector<double> > sfVec( m_outputSystNamesEff );
       if ( !sfVec.isAvailable( *mu_itr ) ) {
	 sfVec( *mu_itr ) = std::vector<double>();
       }
       //
       // obtain efficiency SF
       //
       float effSF(0.0);
       if ( m_MuonEffSFTool->getEfficiencyScaleFactor( *mu_itr, effSF ) != CP::CorrectionCode::Ok ) {
         Error( "execute()", "Problem in getEfficiencyScaleFactor");
         return EL::StatusCode::FAILURE;
       }
       //
       // Add it to decoration vector
       //
       sfVec( *mu_itr ).push_back(effSF);

       if ( m_debug ) { Info( "execute", "===>>> Resulting efficiency SF: %f for systematic: %s ", effSF, syst_it.name().c_str()); }

    } // close muon loop

  }  // close loop on efficiency SF systematics

  //
  // Trigger SFs - this is a per-event weight
  //
  SG::AuxElement::Decorator< std::vector<double> > triggerSFVec( m_outputSystNamesTrig );

  // loop over available systematics for this tool - remember: syst == EMPTY_STRING --> nominal
  for ( const auto& syst_it : m_systListTrig ) {

    //
    // Create the name of the SF weight to be recorded
    //   template:  SYSNAME_TrigEff_SF
    //
    std::string sfName = "TrigEff_SF";
    if ( !syst_it.name().empty() ) {
       std::string prepend = syst_it.name() + "_";
       sfName.insert( 0, prepend );
    }
    if ( m_debug ) { Info("execute()", "SF decoration name is: %s", sfName.c_str()); }
    sysVariationNamesTrig->push_back(sfName);

    // apply syst
    if ( m_MuonTrigSFTool->applySystematicVariation(syst_it) != CP::SystematicCode::Ok ) {
      Error("initialize()", "Failed to configure MuonTriggerScaleFactors for systematic %s", syst_it.name().c_str());
      return EL::StatusCode::FAILURE;
    }
    if ( m_debug ) { Info("execute()", "Successfully applied systematic: %s", syst_it.name().c_str()); }

    // and now apply trigger SF!
    unsigned int nMuons = inputMuons->size();

    if ( m_debug ) { Info( "execute", "Number of muons : %u", nMuons); }
    //
    // obtain trigger SF
    //
    double triggerSF(0.0);
    /*
    if ( nMuons > 0 ) {

      if ( m_MuonTrigSFTool->getTriggerScaleFactor( *inputMuons, triggerSF ) != CP::CorrectionCode::Ok ) {
        Error( "execute()", "Problem in getTriggerScaleFactor");
        return EL::StatusCode::FAILURE;
      }
      if ( m_debug ) { Info( "execute()", "Nominal trigger scaleFactor (single trigger) = %g", triggerSF ); }

      if ( nMuons > 1 ) {
        if ( !m_SinglePlusDiMuTrig.empty() ) {
          if ( m_MuonTrigSFTool->getTriggerScaleFactor( *inputMuons, triggerSF, m_SinglePlusDiMuTrig ) != CP::CorrectionCode::Ok ) {
	    Error( "execute()", "Problem in getTriggerScaleFactor");
	    return EL::StatusCode::FAILURE;
          }
          if ( m_debug ) { Info( "execute()", "Nominal trigger scaleFactor (single + di-muon trigger) = %g", triggerSF ); }
        }
      }
    }
    */
    //
    // Add it to decoration vector
    //
    triggerSFVec( *eventInfo ).push_back(triggerSF);

  }  // close loop on trigger SF systematics

  //
  // add list of efficiency and trigger SF systematics names to TStore
  //
  // NB: we need to make sure that this is not pushed more than once in TStore!
  // This will be the case when this executeSF() function gets called for every syst varied input container,
  // e.g. the different SC containers w/ calibration systematics upstream.
  //
  // Use the counter defined in execute() to check this is done only once
  //
  if ( countSyst == 0 ) {

    RETURN_CHECK( "MuonEfficiencyCorrector::execute()", m_store->record( sysVariationNamesEff, m_outputSystNamesEff), "Failed to record vector of systematic names for efficiency SF" );
    RETURN_CHECK( "MuonEfficiencyCorrector::execute()", m_store->record( sysVariationNamesTrig, m_outputSystNamesTrig), "Failed to record vector of systematic names for trigger SF" );

  }

  return EL::StatusCode::SUCCESS;
}
