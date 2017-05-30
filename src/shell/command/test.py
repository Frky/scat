#-*- coding: utf-8 -*-

from .i_command import ICommand
from src.shell.test.accuracy import TestAccuracy
from src.shell.utils import complete_bin, complete_path, checkpath

class TestCmd(ICommand):
    """

    """

    def __init__(self, test_conf, pintools, logdir, *args, **kwargs):
        self.__conf_path = test_conf
        self.__pintools = pintools
        self.__logdir = logdir
        super(TestCmd, self).__init__(*args, **kwargs)

    def run(self, s, *args, **kwargs):
        split = s.split(" ")

        if len(split) < 1:
            return 
        
        command = split[0]

        if len(split) > 1:
            subcommand = split[1]
        else:
            subcommand = None

        # Test accuracy
        if command == "accuracy":
            TestAccuracy(self.__conf_path, self.__pintools["arity"], self.__pintools["type"], self.__logdir).run(subcommand)
        return
