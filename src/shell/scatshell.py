#-*- coding: utf-8 -*-

from cmd2 import Cmd
from confiture import Confiture, ConfigFileError
# from datetime import datetime

from src.shell.exceptions import PintoolFileNotFound

# from src.shell.command.memcomb import MemComb
# from src.shell.command.memuaf import MemUAF
# from src.shell.command.couple import Couple
from src.shell.command.chart import ChartCmd
from src.shell.command.checkconfig import CheckConfigCmd
from src.shell.command.make import MakeCmd
from src.shell.command.display import DisplayCmd
from src.shell.command.parsedata import ParseDataCmd
from src.shell.command.accuracy import AccuracyCmd
from src.shell.command.launch import LaunchCmd
from src.shell.command.memcomb import MemCombCmd
from src.shell.command.couple import CoupleCmd
from src.shell.command.test import TestCmd
from src.shell.command.mismatch import MismatchCmd
from src.shell.log.log import LogManager

from src.shell.pin.pintool import Pintool
# from src.shell.test import ScatTest

class ScatShell(Cmd):

    prompt = 'scat > '

    def __init__(self, config_path="config/config.yaml"):
        conf = Confiture("config/templates/general.yaml")
        # Parse configuration file and get result
        self.__config = conf.check_and_get(config_path)
        self.__log_manager = LogManager(self.__config)
        # Set the log directory
        self.__logdir = self.__config["log"]["path"]

        # Available pintools
        self.__pintools = dict()
        for pintool in self.__config["pintool"]:
            # Check the needed two arguments
            req = ["src", "obj"]
            for r in req:
                if r not in self.__config["pintool"][pintool].keys():
                    raise PintoolFileNotFound("{0} file not found for pintool {1}".format(r, pintool))
            src = self.__config["pintool"][pintool]["src"]
            obj = self.__config["pintool"][pintool]["obj"]

            # Create pintool object
            pintool_obj = Pintool(
                name=pintool,
                src_path=src,
                obj_path=obj,
                pinconf=self.__config["pin"],
                log_manager=self.__log_manager
            )
            self.__pintools[pintool] = pintool_obj

        # Enable commands
        self.__cmds = dict()
        self.__cmds["checkconfig"] = CheckConfigCmd(
            log_manager=self.__log_manager,
            pinpath=self.__config["pin"]["bin"],
        )
        self.__cmds["make"] = MakeCmd(
            pintools=self.__pintools,
        )

        self.__cmds["display"] = DisplayCmd(
            pintools=self.__pintools,
            log_manager=self.__log_manager,
        )
        self.__cmds["parsedata"] = ParseDataCmd()
        self.__cmds["accuracy"] = AccuracyCmd(
            pintools=self.__pintools,
            log_manager=self.__log_manager,
        )
        self.__cmds["mismatch"] = MismatchCmd(
            pintools=self.__pintools,
            log_manager=self.__log_manager,
        )
        self.__cmds["launch"] = LaunchCmd(
            pintools=self.__pintools,
        )
        self.__cmds["couple"] = CoupleCmd(
            pintools=self.__pintools,
            log_manager=self.__log_manager,
        )
        self.__cmds["memcomb"] = MemCombCmd(
            pintools=self.__pintools,
            log_manager=self.__log_manager,
        )
        if "param" in self.__config["test"].keys():
            self.__cmds["test"] = TestCmd(
                test_conf=self.__config["test"]["desc"],
                param=self.__config["test"]["param"],
                pintools=self.__pintools,
                logdir=self.__logdir,
                resdir=self.__config["test"]["res"],
            )
            self.__cmds["chart"] = ChartCmd(
                resdir=self.__config["test"]["res"],
                conf=self.__config["test"]["param"],
            )

            # Link methods to scat shell
        for cmd, obj in self.__cmds.items():
            setattr(self.__class__, "do_" + cmd, obj.run)
            setattr(self.__class__, "help_" + cmd, obj.help)
            setattr(self.__class__, "complete_" + cmd, obj.complete)

        # Init shell
        Cmd.__init__(self)

        # Read history from file
        try:
            with open(".history", "r+") as history_file:
                line = history_file.readline()
                while line != '':
                    self.history.append(line[:-1])
                    line = history_file.readline()
        except IOError:
            pass

    def do_exit(self, *args):
        """
            Redefine exit function to log the history of commands
        """
        # Write new commands in .history
        with open(".history", "w") as history_file:
            nb_del = max(0,len(self.history)-5)
            for line in self.history[nb_del:]:
                history_file.write(line+'\n')

        return True

    do_q = do_exit
    do_quit = do_exit

    def emptyline(self):
        pass
