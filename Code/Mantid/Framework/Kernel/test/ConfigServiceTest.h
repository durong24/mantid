#ifndef MANTID_CONFIGSERVICETEST_H_
#define MANTID_CONFIGSERVICETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/TestChannel.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/FacilityInfo.h"

#include <Poco/Path.h>
#include <Poco/File.h>
#include <boost/shared_ptr.hpp>
#include <string>
#include <fstream>

#include <Poco/NObserver.h>
#include <Poco/Environment.h>
#include <Poco/File.h>

using namespace Mantid::Kernel;
using Mantid::TestChannel;

class ConfigServiceTest : public CxxTest::TestSuite
{
public: 

  void testLogging()
  {

    // Force the setting to "notice" in case the developer uses a different level.
    Mantid::Kernel::Logger::setLevelForAll(Poco::Message::PRIO_NOTICE);

    //attempt some logging
    Logger log1("logTest");

    TS_ASSERT_THROWS_NOTHING(log1.debug("a debug string"));
    TS_ASSERT_THROWS_NOTHING(log1.information("an information string"));
    TS_ASSERT_THROWS_NOTHING(log1.information("a notice string"));
    TS_ASSERT_THROWS_NOTHING(log1.warning("a warning string"));
    TS_ASSERT_THROWS_NOTHING(log1.error("an error string"));
    TS_ASSERT_THROWS_NOTHING(log1.fatal("a fatal string"));

    TS_ASSERT_THROWS_NOTHING(
    log1.fatal()<<"A fatal message from the stream operators " << 4.5 << std::endl;
    log1.error()<<"A error message from the stream operators " << -0.2 << std::endl;
    log1.warning()<<"A warning message from the stream operators " << 999.99 << std::endl;
    log1.notice()<<"A notice message from the stream operators " << 0.0 << std::endl;
    log1.information()<<"A information message from the stream operators " << -999.99 << std::endl;
    log1.debug()<<"A debug message from the stream operators " << 5684568 << std::endl;


    );

    //checking the level - this should be set to debug in the config file
    //therefore this should only return false for debug
    TS_ASSERT(log1.is(Poco::Message::PRIO_DEBUG) == false); //debug
    TS_ASSERT(log1.is(Poco::Message::PRIO_INFORMATION)==false); //information
    TS_ASSERT(log1.is(Poco::Message::PRIO_NOTICE)); //notice
    TS_ASSERT(log1.is(Poco::Message::PRIO_WARNING)); //warning
    TS_ASSERT(log1.is(Poco::Message::PRIO_ERROR)); //error
    TS_ASSERT(log1.is(Poco::Message::PRIO_FATAL)); //fatal
  }

  void testEnabled()
  {
    //attempt some logging
    Logger log1("logTestEnabled");
    TS_ASSERT(log1.getEnabled());
    TS_ASSERT_THROWS_NOTHING(log1.fatal("a fatal string with enabled=true"));
    TS_ASSERT_THROWS_NOTHING(log1.fatal()<<"A fatal message from the stream operators with enabled=true " << 4.5 << std::endl;);
    
    TS_ASSERT_THROWS_NOTHING(log1.setEnabled(false));
    TS_ASSERT(!log1.getEnabled());
    TS_ASSERT_THROWS_NOTHING(log1.fatal("YOU SHOULD NEVER SEE THIS"));
    TS_ASSERT_THROWS_NOTHING(log1.fatal()<<"YOU SHOULD NEVER SEE THIS VIA A STREAM" << std::endl;);
    
    TS_ASSERT_THROWS_NOTHING(log1.setEnabled(true));
    TS_ASSERT(log1.getEnabled());
    TS_ASSERT_THROWS_NOTHING(log1.fatal("you are allowed to see this"));
    TS_ASSERT_THROWS_NOTHING(log1.fatal()<<"you are allowed to see this via a stream" << std::endl;);

  }

  void testLogLevelOffset()
  {
    //attempt some logging
    Logger log1("logTestOffset");
    log1.setLevelOffset(0);
    TS_ASSERT_THROWS_NOTHING(log1.fatal("a fatal string with offset 0"));
    log1.setLevelOffset(-1);
    TS_ASSERT_THROWS_NOTHING(log1.fatal("a fatal string with offset -1 should still be fatal"));
    TS_ASSERT_THROWS_NOTHING(log1.information("a information string with offset -1 should be notice"));
    log1.setLevelOffset(1);
    TS_ASSERT_THROWS_NOTHING(log1.fatal("a fatal string with offset 1 should be critical"));
    TS_ASSERT_THROWS_NOTHING(log1.notice("a notice string with offset 1 should be information"));
    TS_ASSERT_THROWS_NOTHING(log1.debug("a debug string with offset 1 should be debug"));    
    log1.setLevelOffset(999);
    TS_ASSERT_THROWS_NOTHING(log1.fatal("a fatal string with offset 999 should  be trace"));
    TS_ASSERT_THROWS_NOTHING(log1.notice("a notice string with offset 999 should be trace"));
    TS_ASSERT_THROWS_NOTHING(log1.debug("a debug string with offset 999 should be trace"));
  }

  void testDefaultFacility()
  {
    TS_ASSERT_THROWS_NOTHING(ConfigService::Instance().getFacility() );
//
    ConfigService::Instance().setFacility("ISIS");
    const FacilityInfo& fac = ConfigService::Instance().getFacility();
    TS_ASSERT_EQUALS(fac.name(),"ISIS");

    ConfigService::Instance().setFacility("SNS");
    const FacilityInfo& fac1 = ConfigService::Instance().getFacility();
    TS_ASSERT_EQUALS(fac1.name(),"SNS");

//    // Non existent facility
//    TS_ASSERT_THROWS(ConfigService::Instance().setFacility(""), Mantid::Kernel::Exception::NotFoundError);

  }

  void testFacilityList()
  {
    std::vector<FacilityInfo*> facilities = ConfigService::Instance().getFacilities();
    std::vector<std::string> names = ConfigService::Instance().getFacilityNames();

    TS_ASSERT_LESS_THAN(0,names.size());
    TS_ASSERT_EQUALS(facilities.size(),names.size());
    auto itFacilities = facilities.begin();
    auto itNames = names.begin();
    for (; itFacilities != facilities.end(); ++itFacilities,++itNames)
    {
      TS_ASSERT_EQUALS(*itNames, (**itFacilities).name());
    }


  }

  void testInstrumentSearch()
  {
    // Set a default facility
    ConfigService::Instance().setFacility("SNS");

    // Try and find some instruments from a facility
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("BASIS").name(),"BASIS");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("REF_L").name(),"REF_L");

    // Now find some from other facilities
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("OSIRIS").name(),"OSIRIS");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("BIOSANS").name(),"BIOSANS");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("NGSANS").name(),"NGSANS");

    // Check we throw the correct error for a nonsense beamline.
    //TS_ASSERT_THROWS(ConfigService::Instance().getInstrument("MyBeamline").name(), NotFoundError);

    // Now find by using short name
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("BSS").name(), "BASIS");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("MAR").name(), "MARI");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("PG3").name(), "POWGEN");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("OSI").name(), "OSIRIS");
//    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("HiResSANS").name(), "GPSANS");

    // Now find some with the wrong case
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("baSis").name(),"BASIS");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("TOPaZ").name(),"TOPAZ");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("Seq").name(),"SEQUOIA");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("eqsans").name(),"EQ-SANS");

    // Set the default instrument
    ConfigService::Instance().setString("default.instrument", "OSIRIS");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("").name(), "OSIRIS");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument().name(), "OSIRIS");

  }

  void TestSystemValues()
  {
    //we cannot test the return values here as they will differ based on the environment.
    //therfore we will just check they return a non empty string.
    std::string osName = ConfigService::Instance().getOSName();
    TS_ASSERT_LESS_THAN(0, osName.length()); //check that the string is not empty
    std::string osArch = ConfigService::Instance().getOSArchitecture();
    TS_ASSERT_LESS_THAN(0, osArch.length()); //check that the string is not empty
    std::string osCompName = ConfigService::Instance().getComputerName();
    TS_ASSERT_LESS_THAN(0, osCompName.length()); //check that the string is not empty
    TS_ASSERT_LESS_THAN(0, ConfigService::Instance().getOSVersion().length()); //check that the string is not empty
    TS_ASSERT_LESS_THAN(0, ConfigService::Instance().getCurrentDir().length()); //check that the string is not empty
//        TS_ASSERT_LESS_THAN(0, ConfigService::Instance().getHomeDir().length()); //check that the string is not empty
    TS_ASSERT_LESS_THAN(0, ConfigService::Instance().getTempDir().length()); //check that the string is not empty
  }

  void TestCustomProperty()
  {
    //Mantid.legs is defined in the properties script as 6
    std::string countString = ConfigService::Instance().getString("ManagedWorkspace.DataBlockSize");
    TS_ASSERT_EQUALS(countString, "4000");
  }

   void TestCustomPropertyAsValue()
  {
    //Mantid.legs is defined in the properties script as 6
    int value = 0;
    ConfigService::Instance().getValue("ManagedWorkspace.DataBlockSize",value);
    double dblValue = 0;
    ConfigService::Instance().getValue("ManagedWorkspace.DataBlockSize",dblValue);

    TS_ASSERT_EQUALS(value, 4000);
    TS_ASSERT_EQUALS(dblValue, 4000.0);
  }

  void TestMissingProperty()
  {
    //Mantid.noses is not defined in the properties script 
    std::string noseCountString = ConfigService::Instance().getString("mantid.noses");
    //this should return an empty string

    TS_ASSERT_EQUALS(noseCountString, "");
  }

  void TestRelativeToAbsolute()
  {
    //std::string path = ConfigService::Instance().getString("defaultsave.directory");
    //TS_ASSERT( Poco::Path(path).isAbsolute() );
  } 

  void TestAppendProperties()
  {

    //This should clear out all old properties
    const std::string propfilePath = ConfigService::Instance().getDirectoryOfExecutable();
    const std::string propfile = propfilePath + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);
    //this should return an empty string
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.noses"), "");
    //this should pass
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.legs"), "6");
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.thorax"), "1");

    //This should append a new properties file properties
    ConfigService::Instance().updateConfig(propfilePath+"MantidTest.user.properties",true);
    //this should now be valid
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.noses"), "5");
    //this should have been overridden
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.legs"), "76");
    //this should have been left alone
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.thorax"), "1");

    //This should clear out all old properties
    ConfigService::Instance().updateConfig(propfile);
    //this should return an empty string
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.noses"), "");
    //this should pass
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.legs"), "6");
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.thorax"), "1");

  }

  void testSaveConfigCleanFile()
  {
     const std::string propfile = ConfigService::Instance().getDirectoryOfExecutable() 
      + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);

    const std::string filename("user.settings");
  
    // save any previous changed settings to make sure we're on a clean slate
    ConfigService::Instance().saveConfig(filename);

    Poco::File prop_file(filename);
    // Start with a clean state
    if( prop_file.exists() ) prop_file.remove();

    ConfigServiceImpl& settings = ConfigService::Instance();
    TS_ASSERT_THROWS_NOTHING(settings.saveConfig(filename));

    // No changes yet, file exists but is blank
    TS_ASSERT_EQUALS(prop_file.exists(), true);
    std::string contents = readFile(filename);
    TS_ASSERT(contents.empty());

    runSaveTest(filename,"11");
  }

  void testSaveConfigExistingSettings()
  {

    const std::string filename("user.settings");
    Poco::File prop_file(filename);
    if( prop_file.exists() ) prop_file.remove();

    std::ofstream writer(filename.c_str(),std::ios_base::trunc);
    writer << "mantid.legs = 6";
    writer.close();

    runSaveTest(filename,"13");
  }

  void testLoadChangeLoadSavesOriginalValueIfSettingExists()
  {
    const std::string filename("user.settingsLoadChangeLoad");
    Poco::File prop_file(filename);
    if( prop_file.exists() ) prop_file.remove();
    const std::string value("15");
    std::ofstream writer(filename.c_str());
    writer << "mantid.legs = " << value << "\n";
    writer.close();

    const std::string propfile = ConfigService::Instance().getDirectoryOfExecutable() 
      + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);
    ConfigService::Instance().setString("mantid.legs", value);
    ConfigService::Instance().updateConfig(propfile, false, false);

    ConfigService::Instance().saveConfig(filename);

    const std::string contents = readFile(filename);
    TS_ASSERT_EQUALS(contents, "mantid.legs=6\n");

    prop_file.remove();

  }

  void testLoadChangeClearSavesOriginalPropsFile()
  {
    // Backup current user settings
    ConfigServiceImpl & settings = ConfigService::Instance();
    const std::string userFileBackup = settings.getUserFilename() + ".unittest";
    try
    {
      Poco::File userFile(settings.getUserFilename());
      userFile.moveTo(userFileBackup);
    }
    catch(Poco::Exception&){}

    const std::string propfile = ConfigService::Instance().getDirectoryOfExecutable() 
      + "MantidTest.properties";
    settings.updateConfig(propfile);
    settings.setString("mantid.legs", "15");

    settings.reset();

    const std::string contents = readFile(settings.getUserFilename());
    // No mention of mantid.legs but not empty
    TS_ASSERT(!contents.empty());
    TS_ASSERT(contents.find("mantid.legs") == std::string::npos);


    try
    {
      Poco::File backup(userFileBackup);
      backup.moveTo(settings.getUserFilename());
    }
    catch(Poco::Exception &) {}
        
  }

  void testSaveConfigWithPropertyRemoved()
  {
    const std::string filename("user.settings.testSaveConfigWithPropertyRemoved");
    Poco::File prop_file(filename);
    if( prop_file.exists() ) prop_file.remove();

    std::ofstream writer(filename.c_str(),std::ios_base::trunc);
    writer << "mantid.legs = 6" << "\n";
    writer << "\n";
    writer << "mantid.thorax = 10\n";
    writer << "# This comment line\n";
    writer << "key.withnospace=5\n";
    writer << "key.withnovalue";
    writer.close();

    ConfigService::Instance().updateConfig(filename, false, false);

    std::string rootName = "mantid.thorax";
    ConfigService::Instance().remove(rootName);
    TS_ASSERT_EQUALS(ConfigService::Instance().hasProperty(rootName), false);
    rootName = "key.withnovalue";
    ConfigService::Instance().remove(rootName);
    TS_ASSERT_EQUALS(ConfigService::Instance().hasProperty(rootName), false);

    ConfigService::Instance().saveConfig(filename);

    // Test the entry
    std::ifstream reader(filename.c_str(), std::ios::in);
    if( reader.bad() )
    {
      TS_FAIL("Unable to open config file for saving");
    }
    std::string line("");
    std::map<int, std::string> prop_lines;
    int line_index(0);
    while(getline(reader, line))
    {
      prop_lines.insert(std::make_pair(line_index, line));
      ++line_index;
    }
    reader.close();

    TS_ASSERT_EQUALS(prop_lines.size(), 4);
    TS_ASSERT_EQUALS(prop_lines[0], "mantid.legs=6");
    TS_ASSERT_EQUALS(prop_lines[1], "");
    TS_ASSERT_EQUALS(prop_lines[2], "# This comment line");
    TS_ASSERT_EQUALS(prop_lines[3], "key.withnospace=5");

    // Clean up
    prop_file.remove();
  }

  void testSaveConfigWithLineContinuation()
  {
    /*const std::string propfile = ConfigService::Instance().getDirectoryOfExecutable() 
      + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);*/

    const std::string filename("user.settingsLineContinuation");
    Poco::File prop_file(filename);
    if( prop_file.exists() ) prop_file.remove();

    ConfigServiceImpl& settings = ConfigService::Instance();
    
    std::ofstream writer(filename.c_str(),std::ios_base::trunc);
    writer << 
      "mantid.legs=6\n\n"
      "search.directories=/test1;\\\n"
      "/test2;/test3;\\\n"
      "/test4\n";
    writer.close();

    ConfigService::Instance().updateConfig(filename, true, false);

    TS_ASSERT_THROWS_NOTHING(settings.setString("mantid.legs", "15"));

    TS_ASSERT_THROWS_NOTHING(settings.saveConfig(filename));
    // Should exist
    TS_ASSERT_EQUALS(prop_file.exists(), true);

    // Test the entry
    std::ifstream reader(filename.c_str(), std::ios::in);
    if( reader.bad() )
    {
      TS_FAIL("Unable to open config file for saving");
    }
    std::string line("");
    std::map<int, std::string> prop_lines;
    int line_index(0);
    while(getline(reader, line))
    {
      prop_lines.insert(std::make_pair(line_index, line));
      ++line_index;
    }
    reader.close();

    TS_ASSERT_EQUALS(prop_lines.size(), 3);
    TS_ASSERT_EQUALS(prop_lines[0], "mantid.legs=15");
    TS_ASSERT_EQUALS(prop_lines[1], "");
    TS_ASSERT_EQUALS(prop_lines[2], "search.directories=/test1;/test2;/test3;/test4");

    // Clean up
    //prop_file.remove();
  }

  // Test that the ValueChanged notification is sent
  void testNotifications()
  {
    Poco::NObserver<ConfigServiceTest, ConfigServiceImpl::ValueChanged> changeObserver(*this, &ConfigServiceTest::handleConfigChange);
    m_valueChangedSent = false;

    ConfigServiceImpl& settings = ConfigService::Instance();

    TS_ASSERT_THROWS_NOTHING(settings.addObserver(changeObserver));

    settings.setString("default.facility", "SNS");

    TS_ASSERT(m_valueChangedSent);
    TS_ASSERT_EQUALS(m_key, "default.facility");
    TS_ASSERT_DIFFERS(m_preValue, m_curValue);
    TS_ASSERT_EQUALS(m_curValue, "SNS");
  }


  void testGetKeysWithValidInput()
  {
    const std::string propfile = ConfigService::Instance().getDirectoryOfExecutable() 
      + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);

    // Returns all subkeys with the given root key
    std::vector<std::string> keyVector = ConfigService::Instance().getKeys("workspace.sendto");
    TS_ASSERT_EQUALS(keyVector.size(), 4);
    TS_ASSERT_EQUALS(keyVector[0], "1");
    TS_ASSERT_EQUALS(keyVector[1], "2");
    TS_ASSERT_EQUALS(keyVector[2], "3");
    TS_ASSERT_EQUALS(keyVector[3], "4");
  }

  void testGetKeysWithZeroSubKeys()
  {
     const std::string propfile = ConfigService::Instance().getDirectoryOfExecutable() 
      + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);

    std::vector<std::string> keyVector = ConfigService::Instance().getKeys("mantid.legs");
    TS_ASSERT_EQUALS(keyVector.size(), 0);
    std::vector<std::string> keyVector2 = ConfigService::Instance().getKeys("mantidlegs");
    TS_ASSERT_EQUALS(keyVector2.size(), 0);
  }

  void testGetKeysWithEmptyPrefix()
  {
     const std::string propfile = ConfigService::Instance().getDirectoryOfExecutable() 
      + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);

    //Returns all *root* keys, i.e. unique keys left of the first period
    std::vector<std::string> keyVector = ConfigService::Instance().getKeys(""); 
    // The 4 unique in the file and the ConfigService always sets a datasearch.directories key on creation
    TS_ASSERT_EQUALS(keyVector.size(), 5);
  }

  void testRemovingProperty()
  {
    const std::string propfile = ConfigService::Instance().getDirectoryOfExecutable() 
      + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);

    std::string rootName = "mantid.legs";
    bool mantidLegs = ConfigService::Instance().hasProperty(rootName);
    TS_ASSERT_EQUALS(mantidLegs, true);
    
    //Remove the value from the key and test again
    ConfigService::Instance().remove(rootName);
    mantidLegs = ConfigService::Instance().hasProperty(rootName);
    TS_ASSERT_EQUALS(mantidLegs, false);

  }

protected:
  bool m_valueChangedSent;
  std::string m_key;
  std::string m_preValue;
  std::string m_curValue;
  void handleConfigChange(const Poco::AutoPtr<Mantid::Kernel::ConfigServiceImpl::ValueChanged>& pNf)
  {
    m_valueChangedSent = true;
    m_key = pNf->key();
    m_preValue = pNf->preValue();
    m_curValue = pNf->curValue();
  }

private:
  void runSaveTest(const std::string& filename, const std::string& legs)
  {
    ConfigServiceImpl& settings = ConfigService::Instance();
    // Make a change and save again
    std::string key("mantid.legs");
    std::string value(legs);
    TS_ASSERT_THROWS_NOTHING(settings.setString(key, value));
    TS_ASSERT_THROWS_NOTHING(settings.saveConfig(filename));

    // Should exist
    Poco::File prop_file(filename);
    TS_ASSERT_EQUALS(prop_file.exists(), true);

    // Test the entry
    std::ifstream reader(filename.c_str(), std::ios::in);
    if( reader.bad() )
    {
      TS_FAIL("Unable to open config file for saving");
    }
    std::string line("");
    while(std::getline(reader, line))
    {
      if( line.empty() ) continue;
      else break;
    }
    reader.close();

    std::string key_value = key + "=" + value;
    TS_ASSERT_EQUALS(line, key_value);

    // Clean up
    prop_file.remove();

  }

  std::string readFile(const std::string & filename)
  {
    std::ifstream reader(filename.c_str());
    return std::string((std::istreambuf_iterator<char>(reader)),
                        std::istreambuf_iterator<char>());
  }
  
};

#endif /*MANTID_CONFIGSERVICETEST_H_*/
