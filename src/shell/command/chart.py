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

    def run(self, s, *args, **kwargs):
        inf, param = s.split(" ")
        defaults = dict()
        for k, v in self.__conf[inf][param].items():
            if k not in ["min", "max", "step"]:
                defaults[k] = v
        if inf == "arity":
            chart = ArityChart(self.__resdir + "/" + inf + ".res")
        else:
            chart = TypeChart(self.__resdir + "/" + inf + ".res")
        chart.draw(chart.get(param, defaults), param)
        return 

