#-*- coding: utf-8 -*-

from .i_command import ICommand

from src.shell.chart.chart import ArityChart

class ChartCmd(ICommand):
    """

    """

    def __init__(self, resdir, *args, **kwargs):
        self.__resdir = resdir
        super(ChartCmd, self).__init__(*args, **kwargs)

    def run(self, s, *args, **kwargs):
        inf, param = s.split(" ")
        chart = ArityChart(self.__resdir + "/" + inf + ".res")
        chart.draw(chart.get(param))
        return 

