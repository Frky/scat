#-*- coding: utf-8 -*-

from .i_command import ICommand

from src.shell.chart.arity import ArityChart
from src.shell.chart.type import TypeChart
from src.shell.chart.couple import CoupleChart

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
        super(ChartCmd, self).__init__(*args, **kwargs)

    def __get_res(self, inf="", param="", pgm=""):
        if param == "accuracy":
            return "{}/{}_{}.res".format(self.__resdir, param, inf)
        elif inf == "couple":
            return "{}/{}_{}.res".format(self.__resdir, inf, "general")
        else:
            return "{}/{}_{}.res".format(self.__resdir, pgm, inf)

    def run(self, s, *args, **kwargs):
        split = s.split(" ")
        inf, param = split[:2] 
        if len(split) > 2:
            pgm = split[2]
        else:
            pgm = "test"
        if inf == "arity" or inf == "type":
            if param != "accuracy":
                defaults = dict()
                for k, v in self.__conf[inf][param].items():
                    if k not in ["min", "max", "step"]:
                        defaults[k] = v
                inp = param in ["min_calls", "param_threshold", "min_vals", "max_vals", "addr_threshold"]
                outp = param in ["min_calls", "ret_threshold", "min_vals", "max_vals", "addr_threshold"]
            if inf == "arity":
                chart = ArityChart(self.__get_res(inf, param, pgm))
            else:
                chart = TypeChart(self.__get_res(inf, param, pgm))
            if param == "accuracy":
                chart.draw_accuracy(chart.get_accuracy(), "accuracy")
            elif param == "variability":
                chart.draw_var(chart.get_var(pgm, defaults), "{}_var".format(pgm))
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

