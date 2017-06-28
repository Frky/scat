#-*- coding: utf-8 -*-

from datetime import datetime
from collections import OrderedDict
from confiture import Confiture 
from subprocess import call
import os

from src.shell.analysis.arity import ArityAnalysis
from src.shell.analysis.type import TypeAnalysis
from src.shell.chart.arity import ArityChart
from src.shell.chart.type import TypeChart
from src.shell.data.data import Data
from src.shell.pin.pintool import Pintool
from src.shell.std import Std
from src.shell.test.res.accuracy import AccuracyRes

class TestAccuracy(Std):

    def __init__(self, test_conf, empty, arity, typ, logdir, resdir, *args, **kwargs):
        self.__resdir = resdir
        self.__conf = Confiture("config/templates/empty.yaml").check_and_get(test_conf)
        # Inlcude sub configuration files
        for k, v in self.__conf.items():
            if "config" in v.keys():
                subconf = Confiture("config/templates/empty.yaml").check_and_get(v["config"])
                self.__conf.pop(k)
                self.__conf.update(subconf)
        self.__empty = empty
        self.__arity = arity
        self.__type = typ
        self.__logdir = logdir
        super(TestAccuracy, self).__init__(*args, **kwargs)

    def __run_arity(self):
        res = AccuracyRes()
        ignored = 0
        FNULL = open(os.devnull, "w")
        prev_res = ArityChart(self.__resdir + "/arity.res", self.__conf)
        for pgm, param in OrderedDict(sorted(self.__conf.items(), key=lambda a:a[0])).items():
            if prev_res.contains(pgm):
                continue
            if param["args"] == "":
                ignored += 1
                continue
            if "pre" in param.keys():
                call(param["pre"], stdout=FNULL, shell=True)
            # launch without PIN
            start = datetime.now()
            call(param["bin"] + " " + param["args"], stdout=FNULL, shell=True)
            stop = datetime.now()
            print stop - start
            if "post" in param.keys():
                call(param["post"], stdout=FNULL, shell=True)
            if "pre" in param.keys():
                call(param["pre"], stdout=FNULL, shell=True)
            # launch empty
            self.__empty.launch(param["bin"], [param["args"], "1>/dev/null"], verbose=False)
            if "post" in param.keys():
                call(param["post"], stdout=FNULL, shell=True)
            if "pre" in param.keys():
                call(param["pre"], stdout=FNULL, shell=True)
            # launch program arity
            self.__arity.launch(param["bin"], [param["args"], "1>/dev/null"], verbose=False)
            if "post" in param.keys():
                call(param["post"], stdout=FNULL, shell=True)
            # display arity accuracy
            try:
                if param["data"].endswith("/"):
                    data = Data(param["data"], pgm)
                else:
                    datadir = param["data"][:param["data"].rfind("/") + 1]
                    pgmname = param["data"][param["data"].rfind("/")+1:param["data"].rfind(".")]
                    data = Data(datadir, pgmname)
                data.load()
            except IOError as e:
                self.stderr("error: you must parse source code of \"{0}\" first (use parsedata)".format(pgm))
                continue
            # Get time of execution with PIN and no instrumentation
            with open(self.__empty.get_logfile(pgm, prev=False)) as f:
                empty_time = float(f.read())
            ar = ArityAnalysis(pgm, self.__arity.get_logfile(pgm, prev=False), data)
            res.add(ar.accuracy(get=True, verbose=False, log=self.__resdir + "/" + "arity.res", empty_time=empty_time, no_pin_time=stop - start), pgm=pgm, verbose=True)

        print res
        print "IGNORED: {}".format(ignored)

    def __run_type(self):
        res = AccuracyRes()
        ignored = 0
        FNULL = open(os.devnull, "w")
        prev_res = TypeChart(self.__resdir + "/type.res", self.__conf)
        # used to get times of execution with no instrumentation
        arity_res = ArityChart(self.__resdir + "/arity.res", self.__conf)
        for pgm, param in OrderedDict(sorted(self.__conf.items(), key=lambda a:a[0])).items():
            if prev_res.contains(pgm):
                continue
            # launch program type
            if param["args"] == "":
                ignored += 1
                continue
            if "pre" in param.keys():
                call(param["pre"], stdout=FNULL, shell=True)
            self.__type.launch(param["bin"], [param["args"], "1>/dev/null"])#, verbose=False)
            if "post" in param.keys():
                call(param["post"], stdout=FNULL, shell=True)
            try:
                if param["data"].endswith("/"):
                    data = Data(param["data"], pgm)
                else:
                    datadir = param["data"][:param["data"].rfind("/") + 1]
                    pgmname = param["data"][param["data"].rfind("/")+1:param["data"].rfind(".")]
                    data = Data(datadir, pgmname)
                data.load()
            except IOError:
                self.stderr("error: you must parse source code of \"{0}\" first (use parsedata)".format(pgm))
                continue
            # Get times of execution with no instrumentation
            empty_time = arity_res.get_one(pgm).empty_time
            nopin_time = arity_res.get_one(pgm).nopin_time
            ty = TypeAnalysis(pgm, self.__type.get_logfile(pgm, prev=False), data)
            res.add(ty.accuracy(get=True, verbose=False, log=self.__resdir + "/" + "type.res", empty_time=empty_time, nopin_time=nopin_time), pgm=pgm)

        print res
        print "IGNORED: {}".format(ignored)

    def run(self, subcommand=None):
        if subcommand is None or subcommand == "arity":
            self.__run_arity()
        if subcommand is None or subcommand == "type":
            self.__run_type()

