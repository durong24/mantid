"""
    This class holds all the necessary information to create a reduction script.
    This is a fake version of the Reducer for testing purposes.
"""
import time
import os
from scripter import BaseReductionScripter

class EQSANSReductionScripter(BaseReductionScripter):
    """
        Organizes the set of reduction parameters that will be used to
        create a reduction script. Parameters are organized by groups that
        will each have their own UI representation.
    """
    
    def __init__(self, name="EQSANS"):
        super(EQSANSReductionScripter, self).__init__(name=name)        
    
    def to_script(self, file_name=None):
        """
            Spits out the text of a reduction script with the current state.
            @param file_name: name of the file to write the script to
        """
        script = "# EQSANS reduction script\n"
        script += "# Script automatically generated on %s\n\n" % time.ctime(time.time())
        
        script += "from MantidFramework import *\n"
        script += "mtd.initialise(False)\n"
        script += "from reduction.instruments.sans.sns_command_interface import *\n"
        script += "\n"
        
        for item in self._observers:
            if item.state() is not None:
                script += str(item.state())
        
        xml_process = "None"
        if file_name is None:
            xml_process = os.path.join(self._output_directory, "EQSANS_process.xml")
            xml_process = os.path.normpath(xml_process)
            self.to_xml(xml_process)
            
        script += "SaveIqAscii(process=%r)\n" % xml_process
        script += "Reduce1D()\n"

        if file_name is not None:
            f = open(file_name, 'w')
            f.write(script)
            f.close()
        
        return script
        

    