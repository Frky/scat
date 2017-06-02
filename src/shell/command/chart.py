#-*- coding: utf-8 -*-

from .i_command import ICommand

from src.shell.chart.arity import ArityChart
from src.shell.chart.type import TypeChart

class ChartCmd(ICommand):
    """

    """

    def __init__(self, resdir, conf, *args, **kwargs):
        self.__resdir = resdir
        self.__conf = conf
        super(ChartCmd, self).__init__(*args, **kwargs)

    def __get_res(self, inf="", param="", pgm=""):
        if param == "accuracy":
            return "{}/{}_{}.res".format(self.__resdir, param, inf)
        else:
            return "{}/{}_{}_{}.res".format(self.__resdir, pgm, param, inf)

    def run(self, s, *args, **kwargs):
        split = s.split(" ")
        inf, param = split[:2] 
        if len(split) > 2:
            pgm = split[2]
        else:
            pgm = "test"
        if param != "accuracy":
            defaults = dict()
            for k, v in self.__conf[inf][param].items():
                if k not in ["min", "max", "step"]:
                    defaults[k] = v
        if inf == "arity":
            chart = ArityChart(self.__get_res(inf, param, pgm))
        else:
            chart = TypeChart(self.__get_res(inf, param, pgm))
        if param == "accuracy":
            chart.draw_accuracy(chart.get_accuracy(), "accuracy")
        else:
            chart.draw(chart.get(param, defaults), pgm + "_" + param)

