#-*- coding: utf-8 -*-

from confiture import Confiture, ConfigFileError

from src.shell.data.data import Data
from src.shell.utils import get_pgm_and_inf, get_pgm_list, complete_pgm_pintool
from .i_command import ICommand

class MismatchCmd(ICommand):
    """

    """

    def __init__(self, pintools, logdir, *args, **kwargs):
        self.__logdir = logdir
        self.__pintools = pintools
        super(MismatchCmd, self).__init__(*args, **kwargs)
        return

    def run(self, s, *args, **kwargs):
        try:
            pgm, pintool = get_pgm_and_inf(s, self.__pintools, self.__logdir)
        except ValueError as e:
            raise e
        except KeyError:
             #TODO explicit message (w/ pintool and binary details)
            self.stderr("Pintool error")
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
        pintool.get_analysis(pgm, data).mismatch()


    def complete(self, text, line, begidx, endidx):
        return complete_pgm_pintool(text, line, self.__logdir)

#     def do_mismatch(self, s):
#         """
#             Displays all mismatch for a given program,
#             by comparison with binary and source code.
# 
#         """
#         # Check CLANG configuration
#         conf = Confiture("config/templates/clang.yaml")
#         conf.check("config/config.yaml")
#         try:
#             data = Data(self.config["clang"]["data-path"], pgm)
#             data.load()
#         except IOError:
#             data = None
#         if data is None:
#             self.stderr("error: you must parse source code of \"{0}\" first (use parsedata)".format(pgm))
#             return
# 
#         pintool.get_analysis(pgm, data).mismatch()
