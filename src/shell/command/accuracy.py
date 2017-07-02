#-*- coding: utf-8 -*-

from confiture import Confiture

from src.shell.data.data import Data
from src.shell.utils import *
from .i_command import ICommand

class AccuracyCmd(ICommand):
    """
        usage: accuracy [program (arity | type)]
        \tprogram: the program you have previously analysed

        Analyse the results of inference for a given program,
        by comparison with binary and source code.
        Requires parsedata.
        If no argument is given, display all the eligible programs.
    """

    def __init__(self, pintools, logdir, *args, **kwargs):
        self.__logdir = logdir
        self.__pintools = pintools
        super(AccuracyCmd, self).__init__(*args, **kwargs)
        return

    def run(self, s, *args, **kwargs):
        try:
            pgm, pintool = get_pgm_and_inf(s, self.__pintools, self.__logdir)
        except ValueError as e:
            raise e
        except KeyError:
            self.stderr("Pintool \"{}\" not found".format(s.split()[1]))
            return
        except TypeError:
            self.stderr('Wrong argument(s) detected')
            return

        # Check CLANG configuration
        config = Confiture("config/templates/clang.yaml").check_and_get("config/config.yaml")
        try:
            data = Data(config["clang"]["data-path"], pgm)
            data.load()
        except IOError:
            data = None
        if data is None:
            self.stderr("error: you must parse source code of \"{0}\" first (use parsedata)".format(pgm))
            return
        pintool.get_analysis(pgm, data).accuracy()

    def complete(self, text, line, begidx, endidx):
        return complete_pgm_pintool(text, line, self.__logdir)
