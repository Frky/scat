#-*- coding: utf-8 *-*

import re
import os
from src.shell.pin import INF_ARITY, INF_TYPE, INF_COUPLE
from src.shell.analysis.arity import ArityAnalysis
from src.shell.analysis.type import TypeAnalysis

class Result(object):

    def __init__(self, logdir):
        self.log_dir = logdir


    def compute(self, pgm, infcode, logfile):
        if infcode == INF_ARITY:
            ar = ArityAnalysis(pgm, logfile)
            ar.display()
        elif infcode == INF_TYPE:
            ty = TypeAnalysis(pgm, logfile)
            ty.display()


    def accuracy(self, pgm, infcode, inputfile, data):
        if infcode == INF_ARITY:
            ar = ArityAnalysis(pgm, inputfile, data)
            ar.accuracy()
        elif infcode == INF_TYPE:
            ty = TypeAnalysis(pgm, inputfile, data)
            ty.accuracy()


    def get_pgm_list(self, inf_code=None):
        file_list = [f for f in os.listdir(self.log_dir) if f.endswith("log")]
        pgm_list = set([re.sub("_.*", "", f) for f in file_list]) 
        pgm_inf = list()
        for p in pgm_list:
            inf = set([re.sub("_.*$", "", re.sub("^[^_]*_", "", f)) for f in file_list if f.find(p) >= 0])
            pgm_inf.append((p, list(inf)))
        return pgm_inf

