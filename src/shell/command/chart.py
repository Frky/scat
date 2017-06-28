#-*- coding: utf-8 -*-

from confiture import Confiture
from .i_command import ICommand

from src.shell.chart.arity import ArityChart
from src.shell.chart.type import TypeChart
from src.shell.chart.couple import CoupleChart
from src.shell.chart.alloc import AllocChart

class ChartCmd(ICommand):
    """
        usage: chart <analysis> <parameter> [program]

        Non-optional arguments:
            analysis: the analysis for which you want to get results (either arity or type)
            parameter: the parameter to vary (e.g. min_calls)


        Optional arguments:
            program: program you want to analyse (by default, it gathers all available results)


        Note that if param = "accuracy", it outputs data for every program with default parameter values.

    """

    def __init__(self, resdir, conf, *args, **kwargs):
        self.__resdir = resdir
        self.__conf = conf
        self.__testconf = None
        super(ChartCmd, self).__init__(*args, **kwargs)

    def __get_res(self, inf="", param="", pgm=""):
        if inf == "arity" or inf == "type":
            return "{}/{}.res".format(self.__resdir, inf)
        elif inf == "couple":
            return "{}/{}_{}.res".format(self.__resdir, inf, "general")
        else:
            return "{}/{}_{}.res".format(self.__resdir, pgm, inf)

    def run(self, s, *args, **kwargs):
        split = s.split(" ")

        # Watch for a particular test description file
        if "-t" in split:
            self.__testconf = Confiture("config/templates/empty.yaml").check_and_get(split.pop(split.index("-t") + 1))
            for k, v in self.__testconf.items():
                if "config" in v.keys():
                    subconf = Confiture("config/templates/empty.yaml").check_and_get(v["config"])
                    self.__testconf.pop(k)
                    self.__testconf.update(subconf)

        inf, param = split[:2] 
        if len(split) > 2:
            pgm = split[2]
        else:
            pgm = "test"
        if inf == "arity" or inf == "type":
            if param != "accuracy" and param != "scalability" and param != "overhead":
                defaults = dict()
                for k, v in self.__conf[inf][param].items():
                    if k not in ["min", "max", "step"]:
                        defaults[k] = v
                inp = param in ["min_calls", "param_threshold", "min_vals", "max_vals", "addr_threshold"]
                outp = param in ["min_calls", "ret_threshold", "min_vals", "max_vals", "addr_threshold"]
            if inf == "arity":
                chart = ArityChart(self.__get_res(inf, param, pgm), self.__testconf)
            else:
                chart = TypeChart(self.__get_res(inf, param, pgm), self.__testconf)
            if param == "accuracy":
                chart.draw_accuracy(chart.get_accuracy(), "accuracy")
            elif param == "variability":
                chart.draw_var(chart.get_var(pgm, defaults), "{}_var".format(pgm))
            elif param == "scalability":
                chart.draw_scalability(chart.get_accuracy(), "scalability")
            elif param == "overhead":
                chart.draw_overhead(chart.get_overhead(self.__testconf))
            else:
                chart.draw(chart.get(param, defaults, inp=inp, outp=outp), pgm + "_" + param)
        elif inf == "couple":
            if param == "general":
                chart = CoupleChart(self.__get_res(inf, param, pgm))
                chart.draw_accuracy(chart.get_accuracy(), "general")
            else:
                defaults = dict()
                for k, v in self.__conf[inf][param].items():
                    if k not in ["min", "max", "step"]:
                        defaults[k] = v
                if param == "variability":
                    chart = CoupleChart(self.__get_res(inf, param, pgm))
                    chart.draw_var(chart.get_var(pgm, defaults), "var")
                else:
                    chart = CoupleChart("{}/{}".format(self.__resdir, param))
                    chart.draw(chart.get(param, defaults), param)
        elif inf == "alloc":
            # compute oracle
            oracle = dict()
            if self.__testconf is not None:
                # Inlcude sub configuration files
                for k, v in self.__testconf.items():
                    if "config" in v.keys():
                        subconf = Confiture("config/templates/empty.yaml").check_and_get(v["config"])
                        self.__testconf.pop(k)
                        self.__testconf.update(subconf)
                for pgm, vals in self.__testconf.items():
                    if "oracle" in vals.keys():
                        oracle[pgm] = dict(vals["oracle"])
                        for k, v in oracle[pgm].items():
                            if v is not None:
                                oracle[pgm][k] = v.split(" ")
                            else:
                                oracle[pgm][v] = list()
            if param == "couple" or param == "type":
                chart = AllocChart(oracle, "{}/alloc_{}_general.res".format(self.__resdir, param))
                chart.table()
            elif param == "compare":
                chart = AllocChart(oracle, "{}/alloc_couple_general.res".format(self.__resdir))
                chart.table_cmp(AllocChart(oracle, "{}/alloc_type_general.res".format(self.__resdir)))
                return 
            elif param == "consistency":
                chart = AllocChart(oracle, "{}/alloc_couple_consistency.res".format(self.__resdir))
                chart.draw_consistency()


