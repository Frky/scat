#-*- coding: utf-8 *-*

import re
import os



from src.shell.analysis.arity import ArityAnalysis
from src.shell.analysis.type import TypeAnalysis
from src.shell.analysis.couple import CoupleAnalysis

class Result(object):

    def __init__(self, logdir):
        self.log_dir = logdir


    def compute(self, pgm, pintool):
        logfile = pintool.get_logfile(pgm, prev=False)
        #TODO better than string here
        if str(pintool) == 'arity':
            ar = ArityAnalysis(pgm, logfile)
            ar.display()
        elif str(pintool) == 'type':
            ty = TypeAnalysis(pgm, logfile)
            ty.display()
        elif str(pintool) == 'couple':
            co = CoupleAnalysis(pgm, logfile)
            co.display()


    def accuracy(self, pgm, pintool, data):
        inputfile = pintool.get_logfile(pgm, prev=False)
        if str(pintool) == 'arity':
            ar = ArityAnalysis(pgm, inputfile, data)
            ar.accuracy()
        elif str(pintool) == 'type':
            ty = TypeAnalysis(pgm, inputfile, data)
            ty.accuracy()


    def mismatch(self, pgm, pintool, data):
        inputfile = pintool.get_logfile(pgm, prev=False)
        if str(pintool) == 'arity':
            ar = ArityAnalysis(pgm, inputfile, data)
            ar.mismatch()
        elif str(pintool) == 'type':
            ty = TypeAnalysis(pgm, inputfile, data)
            ty.mismatch()


    def get_pgm_list(self, inf_code=None):
        file_list = [f for f in os.listdir(self.log_dir) if f.endswith("log")]
        pgm_list = set([re.sub("_.*", "", f) for f in file_list])
        pgm_inf = list()
        for p in pgm_list:
            inf = set([re.sub("_.*$", "", re.sub("^[^_]*_", "", f)) for f in file_list if f.find(p) >= 0])
            pgm_inf.append((p, list(inf)))
        return pgm_inf
