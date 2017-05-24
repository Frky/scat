#-*- coding: utf-8 -*-

from subprocess import call

from confiture import Confiture 

from src.shell.analysis.arity import ArityAnalysis
from src.shell.analysis.type import TypeAnalysis
from src.shell.data.data import Data
from src.shell.pin.pintool import Pintool
from collections import OrderedDict

class TestAccuracy(object):

    def __init__(self, test_conf, arity, typ, logdir, *args, **kwargs):
        self.conf = Confiture("config/templates/empty.yaml").check_and_get(test_conf)
        self.__arity = arity
        self.__type = typ
        self.__logdir = logdir
        super(TestAccuracy, self).__init__(*args, **kwargs)

    def __run_arity(self):
        res = AccuracyRes()
        ignored = 0
        for pgm, param in OrderedDict(sorted(self.conf.items(), key=lambda a:a[0])).items():
            if "pre" in param.keys():
                call(param["pre"].split(" "))
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
                call(param["post"].split(" "))

        print res
        print "IGNORED: {}".format(ignored)

    def __run_type(self):
        for pgm, param in OrderedDict(sorted(self.conf.items(), key=lambda a:a[0])).items():
            if "pre" in param.keys():
                call(param["pre"].split(" "))
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
            print ty.accuracy(get=True, verbose=False)
            if "post" in param.keys():
                call(param["post"].split(" "))

    def run(self):
        # self.__run_arity()
        self.__run_type()

class AccuracyRes(object):

    def __init__(self):
        self.__in = {
                        "ok": 0,
                        "less": 0,
                        "more": 0,
                        "tot": 0,
                    }                    
        self.__out = {
                        "ok": 0,
                        "less": 0,
                        "more": 0,
                        "tot": 0,
                    }

    def add(self, res, pgm="", verbose=True):
        self.__in["ok"] += res[0]
        self.__in["more"] += res[2]
        self.__in["less"] += res[3]
        self.__in["tot"] += res[-1]
        self.__out["ok"] += res[1]
        self.__out["more"] += res[4]
        self.__out["less"] += res[5]
        self.__out["tot"] += res[-1]
        if verbose:
            try:
                print "{}: {}/{} {:.2f}% ({} fn, {} fp) - {}/{} {:.2f}%".format(
                        pgm, 
                        res[0],
                        res[-1],
                        res[0]*100./res[-1],
                        res[3],
                        res[2],
                        res[1],
                        res[-1],
                        res[1]*100./res[-1],
                            )
            except Exception:
                print "{}: n.c.".format(pgm)
    def __str__(self):
        return "{}: {}/{} {:.2f}% ({} fn, {} fp) - {}/{} {:.2f}%".format(
                    "TOTAL", 
                    self.__in["ok"], 
                    self.__in["tot"], 
                    self.__in["ok"]*100./self.__in["tot"],
                    self.__in["less"],
                    self.__in["more"],
                    self.__out["ok"], 
                    self.__out["tot"], 
                    self.__out["ok"]*100./self.__out["tot"],
                )

