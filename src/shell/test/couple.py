#-*- coding: utf-8 -*-

from collections import OrderedDict
from confiture import Confiture 
from subprocess import call
import os

from src.shell.couple.couple import Couple
from src.shell.chart.couple import CoupleChart

from .res.couple import CoupleRes

class TestCouple(object):

    def __init__(self, test_conf, pintool, logdir, resdir, *args, **kwargs):
        self.__resdir = resdir
        self.__conf = Confiture("config/templates/empty.yaml").check_and_get(test_conf)
        # Include sub configuration files
        for k, v in self.__conf.items():
            if "config" in v.keys():
                subconf = Confiture("config/templates/empty.yaml").check_and_get(v["config"])
                self.__conf.pop(k)
                self.__conf.update(subconf)
        self.__couple = pintool
        self.__logdir = logdir
        super(TestCouple, self).__init__(*args, **kwargs)

    def run(self, params=None, logname="couple_general.res"):
        FNULL = open(os.devnull, "w")
        prev_res = CoupleChart(self.__resdir + "/" + logname)
        for pgm, param in OrderedDict(sorted(self.__conf.items(), key=lambda a:a[0])).items():
            if prev_res.contains(pgm, params):
                continue
            if param["args"] == "":
                continue
            if "pre" in param.keys():
                call(param["pre"], stdout=FNULL, shell=True)
            if params is not None and "rho" in params.keys():
                min_rho = params.pop("rho")
            else:
                min_rho = 0.8
            if params is not None and "min_vals" in params.keys():
                min_vals = params.pop("min_vals")
            else:
                min_vals = 50
            # launch program couple
            self.__couple.launch(param["bin"], [param["args"], "1>/dev/null"], params=params)#, verbose=False)
            logfile = self.__couple.get_logfile(pgm, prev=False)
            # launch offline computation of couples
            res = Couple(logfile, pgm, verbose=False).run(get=True, log=self.__resdir + "/" + logname, min_rho=min_rho, min_vals=min_vals)
            print "{} | functions: {} - #f: {} - #g: {} - #couples: {}".format(pgm, *res)
            if "post" in param.keys():
                call(param["post"], stdout=FNULL, shell=True)
        return

    def run_params(self, params, name):
        param = filter(lambda a: isinstance(a[1], list), [(k, v) for k, v in params.items()])[0][0]
        vals = params.pop(param)
        for val in vals:
            p = dict()
            p[param] = val
            p.update(params)
            self.run(p, name)

