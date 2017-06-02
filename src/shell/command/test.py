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

        if len(split) < 1:
            return 

        if "-t" in split:
            self.__conf_path = split.pop(split.index("-t") + 1)
            split.pop(split.index("-t"))
        command = split[0]

        # Test accuracy
        if command == "accuracy":
            if len(split) > 1:
                subcommand = split[1]
            else:
                subcommand = None
            TestAccuracy(self.__conf_path, self.__pintools["arity"], self.__pintools["type"], self.__logdir, resdir=self.__resdir).run(subcommand)
        elif command == "param":
            if len(split) < 2 or split[1] not in ["arity", "type"]: 
                self.stderr("Unknown or missing analysis (arity, type) -- aborting")
                return 
            analysis = split[1]
            if len(split) < 3 or split[2] not in self.__param[analysis].keys():
                self.stderr("Unknown or missing parameter -- aborting")
                return
            param = split[2]
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
            test = TestParameter(self.__conf_path, self.__pintools[analysis], resdir=self.__resdir, prev_pintool=prev)
            test.run(params)
        return
