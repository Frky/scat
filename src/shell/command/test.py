#-*- coding: utf-8 -*-

from .i_command import ICommand
from src.shell.test.accuracy import TestAccuracy
from src.shell.test.parameter import TestParameter
from src.shell.utils import complete_bin, complete_path, checkpath

class TestCmd(ICommand):
    """

    """

    def __init__(self, test_conf, param, pintools, logdir, resdir, *args, **kwargs):
        self.__conf_path = test_conf
        self.__param = param
        self.__pintools = pintools
        self.__logdir = logdir
        self.__resdir = resdir
        super(TestCmd, self).__init__(*args, **kwargs)

    def run(self, s, *args, **kwargs):
        split = s.split(" ")

        # Watch for a particular test description file
        # if not specified, the one provided by the configuration file is used
        if "-t" in split:
            self.__conf_path = split.pop(split.index("-t") + 1)
            split.pop(split.index("-t"))

        if len(split) < 1:
            self.help()
            return 

        analysis = split[0]
        
        if analysis == "arity" or analysis == "type":
            if len(split) < 2:
                self.help()
                return 
            param = split[1]
            if param == "accuracy":
                TestAccuracy(
                                self.__conf_path, 
                                self.__pintools["arity"], 
                                self.__pintools["type"], 
                                self.__logdir, 
                                resdir=self.__resdir).run()
            else:
                if param not in self.__param[analysis].keys():
                    self.stderr("unknown parameter: {} -- aborting".format(param))
                vals = list()
                v = self.__param[analysis][param]["min"]
                step = self.__param[analysis][param]["step"]
                while v <= self.__param[analysis][param]["max"]: 
                    vals.append(round(v, 2))
                    v += step
                params = dict()
                params[param] = vals
                for p, v in self.__param[analysis][param].items():
                    if p not in ["min", "max", "step"]:
                        params[p] = v
                if analysis == "arity":
                    prev = None
                else:
                    prev = self.__pintools["arity"]
                test = TestParameter(
                                        self.__conf_path, self.__pintools[analysis], 
                                        resdir=self.__resdir, 
                                        prev_pintool=prev
                                    )
                test.run(params)
        elif analysis == "couple":
            if len(split) < 2:
                self.help()
                return 
            param = split[1]
            if param == "general":
                pass
            else: 
                self.stderr("unknown parameter: {} -- aborting".format(param))
                raise Exception("not implemented yet")
        else:
            self.stderr("unknown analysis: {} -- aborting".format(analysis))
        return
