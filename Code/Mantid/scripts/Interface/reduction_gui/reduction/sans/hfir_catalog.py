"""
    Data catalog for HFIR SANS
"""
from data_cat import DataCatalog as BaseCatalog
from data_cat import DataSet as BaseDataSet
import os
import time

# Check whether Mantid is available
try:
    from MantidFramework import *
    mtd.initialise(False)
    import mantidsimple
    HAS_MANTID = True
except:
    HAS_MANTID = False    

class HFIRDataSet(BaseDataSet):
    TABLE_NAME="hfir_dataset"
    def __init__(self, run_number, title, run_start, duration, sdd):
        super(HFIRDataSet, self).__init__(run_number, title, run_start, duration, sdd)

    @classmethod
    def load_meta_data(cls, file_path, outputWorkspace):
        try:
            mantidsimple.LoadSpice2D(file_path, OutputWorkspace=outputWorkspace)
            return True
        except:
            return False

    @classmethod
    def handle(cls, file_path):
        """
            Return a DB handle for the given file, such as a run number
        """
        handle = os.path.splitext(os.path.basename(file_path))[0]
        if handle=="":
            return None
        return handle
    
    @classmethod
    def read_properties(cls, ws, run, cursor):
        def read_prop(prop):
            try:
                return str(mtd[ws].getRun().getProperty(prop).value)
            except:
                return ""
        def read_series(prop):
            try:
                return float(mtd[ws].getRun().getProperty(prop).getStatistics().mean)
            except:
                return -1
        
        title = read_prop("run_title")
        t_str = read_prop("start_time")
        t = time.strptime(t_str, '%Y-%m-%d %H:%M:%S')
        run_start = time.strftime('%y-%m-%d %H:%M', t)

        duration = read_prop("timer")
        try:
            duration = float(duration)
        except:
            duration = 0
        
        sdd = float(read_prop("sample-detector-distance"))

        d = HFIRDataSet(run, title, run_start, duration, sdd)
        d.insert_in_db(cursor)
        return d
    

class DataCatalog(BaseCatalog):
    extension = "xml"
    data_set_cls = HFIRDataSet

    def __init__(self, replace_db=False):
        super(DataCatalog, self).__init__(replace_db)