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
        super(ChartCmd, self).__init__(*args, **kwargs)

    def __get_res(self, inf="", param="", pgm=""):
        if inf == "arity" or inf == "type":
            return "{}/{}/{}.res".format(self.__resdir, inf, pgm)
        elif inf == "couple":
            return "{}/{}/{}.res".format(self.__resdir, inf, pgm)
        else:
            return "{}/{}_{}.res".format(self.__resdir, pgm, inf)

    def run(self, s, *args, **kwargs):
        split = s.split(" ")

        if len(split) < 3:
            self.help()
            return

        inf, param, test_name = split[:3] 

        if inf == "arity" or inf == "type":
            if param != "accuracy" and param != "overhead" and param != "overhead":
                defaults = dict()
                for k, v in self.__conf[inf][param].items():
                    if k not in ["min", "max", "step"]:
                        defaults[k] = v
                inp = param in ["min_calls", "param_threshold", "min_vals", "max_vals", "addr_threshold"]
                outp = param in ["min_calls", "ret_threshold", "min_vals", "max_vals", "addr_threshold"]
            if inf == "arity":
                chart = ArityChart(self.__get_res(inf, param, test_name))
            else:
                chart = TypeChart(self.__get_res(inf, param, test_name))
            if param == "accuracy":
                chart.draw_accuracy(chart.get_accuracy(), "accuracy")
            elif param == "variability":
                chart.draw_var(chart.get_var(test_name, defaults), "{}_var".format(test_name))
            elif param == "overhead":
                chart.draw_scalability(chart.get_accuracy(), "overhead")
            else:
                chart.draw(chart.get(param, defaults, inp=inp, outp=outp), "{}_{}".format(test_name, param))
        elif inf == "couple":
            if param == "general":
                chart = CoupleChart(self.__get_res(inf, param, test_name))
                chart.draw_accuracy(chart.get_accuracy(), "general")
            else:
                defaults = dict()
                for k, v in self.__conf[inf][param].items():
                    if k not in ["min", "max", "step"]:
                        defaults[k] = v
                if param == "variability":
                    chart = CoupleChart(self.__get_res(inf, param, test_name))
                    chart.draw_var(chart.get_var(test_name, defaults), "var")
                else:
                    chart = CoupleChart("{}/{}".format(self.__resdir, param))
                    chart.draw(chart.get(param, defaults), param)
        elif inf == "alloc":
			# Watch for a particular test description file
			if "-t" in split:
				testconf = Confiture("config/templates/empty.yaml").check_and_get(split.pop(split.index("-t") + 1))
				for k, v in testconf.items():
					if "config" in v.keys():
						subconf = Confiture("config/templates/empty.yaml").check_and_get(v["config"])
						testconf.pop(k)
						testconf.update(subconf)
			# compute oracle
			oracle = dict()
			if testconf is not None:
			    # Inlcude sub configuration files
			    for k, v in testconf.items():
			        if "config" in v.keys():
			            subconf = Confiture("config/templates/empty.yaml").check_and_get(v["config"])
			            testconf.pop(k)
			            testconf.update(subconf)
			    for pgm, vals in testconf.items():
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
			
			
