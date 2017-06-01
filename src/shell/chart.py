#-*- coding: utf-8 -*-

from .i_command import ICommand
from src.shell.test.accuracy import TestAccuracy
from src.shell.test.parameter import TestParameter
from src.shell.utils import complete_bin, complete_path, checkpath

class ChartCmd(ICommand):
    """

    """

    def __init__(self, test_conf, param, pintools, logdir, *args, **kwargs):
        self.__conf_path = test_conf
        self.__param = param
        self.__pintools = pintools
        self.__logdir = logdir
        super(ChartCmd, self).__init__(*args, **kwargs)

    def run(self, s, *args, **kwargs):
        split = s.split(" ")

        if len(split) < 1:
            return 
        
        command = split[0]

        # Test accuracy
        if command == "accuracy":
            if len(split) > 1:
                subcommand = split[1]
            else:
                subcommand = None
            TestAccuracy(self.__conf_path, self.__pintools["arity"], self.__pintools["type"], self.__logdir).run(subcommand)
        elif command == "param":
            if len(split) < 2 or split[1] not in ["arity", "type"]: 
                self.stderr("Unknown or missing analysis (arity, type) -- aborting")
                return 
            analysis = split[1]
            if len(split) < 3 or split[2] not in self.__param[analysis].keys():
                self.stderr("Unknown or missing parameter -- aborting")
                return
            param = split[2]
            vals = map(lambda a: int(a), self.__param[analysis][param].split(" "))
            test = TestParameter(self.__conf_path, self.__pintools[analysis])
            test.run(param, vals)
        return
