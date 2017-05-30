#-*- coding: utf-8 -*-

from collections import OrderedDict
from confiture import Confiture 
from subprocess import call
import os

from src.shell.analysis.arity import ArityAnalysis
from src.shell.analysis.type import TypeAnalysis
from src.shell.data.data import Data
from src.shell.test.res.accuracy import AccuracyRes
from src.shell.pin.pintool import Pintool

class TestAccuracy(object):

    def __init__(self, test_conf, arity, typ, logdir, *args, **kwargs):
        self.__conf = Confiture("config/templates/empty.yaml").check_and_get(test_conf)
        # Inlcude sub configuration files
        for k, v in self.__conf.items():
            if "config" in v.keys():
                subconf = Confiture("config/templates/empty.yaml").check_and_get(v["config"])
                self.__conf.pop(k)
                self.__conf.update(subconf)
        self.__arity = arity
        self.__type = typ
        self.__logdir = logdir
        super(TestAccuracy, self).__init__(*args, **kwargs)

    def __run_arity(self):
        res = AccuracyRes()
        ignored = 0
        FNULL = open(os.devnull, "w")
        for pgm, param in OrderedDict(sorted(self.__conf.items(), key=lambda a:a[0])).items():
            if "pre" in param.keys():
                call(param["pre"], stdout=FNULL, shell=True)
            # launch program arity
            if param["args"] == "":
                ignored += 1
                continue
            self.__arity.launch(param["bin"], [param["args"], "1>/dev/null"])#, verbose=False)
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
            ar = ArityAnalysis(pgm, self.__arity.get_logfile(pgm, prev=False), data)
            res.add(ar.accuracy(get=True, verbose=False), pgm=pgm, verbose=True)
            if "post" in param.keys():
                call(param["post"], stdout=FNULL, shell=True)

        print res
        print "IGNORED: {}".format(ignored)

    def __run_type(self):
        res = AccuracyRes()
        ignored = 0
        FNULL = open(os.devnull, "w")
        for pgm, param in OrderedDict(sorted(self.__conf.items(), key=lambda a:a[0])).items():
            if "pre" in param.keys():
                call(param["pre"], stdout=FNULL, shell=True)
            # launch program type
            if param["args"] == "":
                ignored += 1
                continue
            self.__type.launch(param["bin"], [param["args"], "1>/dev/null"])#, verbose=False)
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
            ty = TypeAnalysis(pgm, self.__type.get_logfile(pgm, prev=False), data)
            res.add(ty.accuracy(get=True, verbose=False), pgm=pgm)
            if "post" in param.keys():
                call(param["post"], stdout=FNULL, shell=True)

        print res
        print "IGNORED: {}".format(ignored)

    def run(self, subcommand=None):
        if subcommand is None or subcommand == "arity":
            self.__run_arity()
        if subcommand is None or subcommand == "type":
            self.__run_type()

