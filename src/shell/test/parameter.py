#-*- coding: utf-8 -*-

from collections import OrderedDict
from confiture import Confiture 
from subprocess import call
import os

from src.shell.analysis.arity import ArityAnalysis
from src.shell.analysis.type import TypeAnalysis
from src.shell.chart.arity import ArityChart
from src.shell.chart.type import TypeChart
from src.shell.data.data import Data
from src.shell.test.res.accuracy import AccuracyRes
from src.shell.pin.pintool import Pintool

class TestParameter(object):

    def __init__(self, test_conf, pintool, resdir, *args, **kwargs):
        self.__resdir = resdir
        self.__testname = os.path.splitext(os.path.basename(test_conf))[0]
        self.__conf = Confiture("config/templates/empty.yaml").check_and_get(test_conf)
        # Include sub configuration files
        for k, v in self.__conf.items():
            if "config" in v.keys():
                subconf = Confiture("config/templates/empty.yaml").check_and_get(v["config"])
                self.__conf.pop(k)
                self.__conf.update(subconf)
        self.__pintool = pintool
        if "prev_pintool" in kwargs.keys():
            self.__prev_pintool = kwargs["prev_pintool"]
        else:
            self.__prev_pintool = None
        self.__prev_treated = list()
        if str(pintool) == "arity":
            self.__res = ArityChart(self.__resdir + "/{}_arity.res".format(self.__testname), self.__conf)
        else:
            self.__res = TypeChart(self.__resdir + "/{}_type.res".format(self.__testname), self.__conf)
        super(TestParameter, self).__init__()

    def __run_one(self, params):
        res = AccuracyRes()
        FNULL = open(os.devnull, "w")
        for pgm, param in OrderedDict(sorted(self.__conf.items(), key=lambda a:a[0])).items():
            if self.__res.contains(pgm, params):
                continue
            if self.__prev_pintool is not None and pgm not in self.__prev_treated:
                if "pre" in param.keys():
                    call(param["pre"], stdout=FNULL, shell=True)
                self.__prev_pintool.launch(param["bin"], [param["args"], "1>/dev/null"], verbose=False)
                if "post" in param.keys():
                    call(param["post"], stdout=FNULL, shell=True)
                self.__prev_treated.append(pgm)
            if "pre" in param.keys():
                call(param["pre"], stdout=FNULL, shell=True)
            # launch program arity
            if param["args"] == "":
                continue
            self.__pintool.launch(param["bin"], [param["args"], "1>/dev/null"], params, verbose=True)
            # display arity accuracy
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
            if str(self.__pintool) == "arity":
                an = ArityAnalysis(pgm, self.__pintool.get_logfile(pgm, prev=False), data)
            else:
                an = TypeAnalysis(pgm, self.__pintool.get_logfile(pgm, prev=False), data)
            res.add(an.accuracy(get=True, verbose=False, log=self.__resdir + "/" + str(self.__testname) + "_" + str(self.__pintool) + ".res"), pgm=pgm, verbose=True)
            if "post" in param.keys():
                call(param["post"], stdout=FNULL, shell=True)

        print res

    def run(self, params):
        param = filter(lambda a: isinstance(a[1], list), [(k, v) for k, v in params.items()])[0][0]
        vals = params.pop(param)
        for val in vals:
            p = dict()
            p[param] = val
            p.update(params)
            self.__run_one(p)
