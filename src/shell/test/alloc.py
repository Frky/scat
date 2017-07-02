#-*- coding: utf-8 -*-

from datetime import datetime

from collections import OrderedDict
from confiture import Confiture 
from subprocess import call
import os

from src.shell.memory.memcomb import MemComb
from src.shell.chart.alloc import AllocChart

from .res.alloc import AllocRes

class TestAlloc(object):

    def __init__(self, test_conf, pintools, logdir, resdir, 
                    alt_prev=False, *args, **kwargs):
        self.__resdir = resdir
        self.__conf = Confiture("config/templates/empty.yaml").check_and_get(test_conf)
        # Include sub configuration files
        for k, v in self.__conf.items():
            if "config" in v.keys():
                subconf = Confiture("config/templates/empty.yaml").check_and_get(v["config"])
                self.__conf.pop(k)
                self.__conf.update(subconf)
        self.__memalloc = pintools["memalloc"]
        self.__type = pintools["type"]
        self.__logdir = logdir
        self.__alt_prev = alt_prev
        super(TestAlloc, self).__init__(*args, **kwargs)

    def run(self, params=None, logname="alloc_general.res", consistency=False):
        FNULL = open(os.devnull, "w")
        prev_res = AllocChart(None, self.__resdir + "/" + logname)
        for pgm, param in OrderedDict(sorted(self.__conf.items(), key=lambda a:a[0])).items():
            if prev_res.contains(pgm):
                continue
            if param["args"] == "":
                continue
            if "pre" in param.keys():
                call(param["pre"], stdout=FNULL, shell=True)
            # launch memalloc
            self.__memalloc.launch(param["bin"], [param["args"], "1>/dev/null"], params=params, alt_prev=self.__alt_prev, verbose=False)
            logfile = self.__memalloc.get_logfile(pgm, prev=False)
            typelogfile = self.__type.get_logfile(pgm, prev=False)
            if self.__alt_prev:
                couplereslogfile = self.__memalloc.get_logfile(pgm, alt_prev=True)
            else:
                couplereslogfile = None
            if "libraries" in param.keys():
                libraries = param["libraries"]
            else:
                libraries = False
            # launch offline computation of memcomb
            res = MemComb(logfile, typelogfile, pgm, coupleres_log_file=couplereslogfile, verbose=False).run(get=True, libraries=libraries, log=self.__resdir + "/" + logname, test_consistency=consistency)
            if res is not None:
                if res[0] is not None and res[0] != "None":
                    alloc = res[0].split(":")
                    alloc[1] = hex(int(alloc[1]))
                else:
                    alloc = "None"
                if res[1] is not None and res[1] != "None":
                    free = res[1].split(":")
                    free[1] = hex(int(free[1]))
                else:
                    free = "None"
                print "{}: ALLOC|FREE {}|{} - ERRORS {}|{} - CALLS {}|{} - ONLINE {}|{} - OFFLINE {}".format(
                        pgm,
                        ":".join(alloc),
                        ":".join(free),
                            res[2][0][0],
                            res[2][0][1],
                            res[2][1][0],
                            res[2][1][1],
                            res[3][0],
                            res[3][1],
                            res[4],
                        )
            if "post" in param.keys():
                call(param["post"], stdout=FNULL, shell=True)
        return

