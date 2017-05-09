#-*- coding: utf-8 -*-

from cmd2 import Cmd
from confiture import Confiture, ConfigFileError
# from datetime import datetime

from src.shell.exceptions import PintoolFileNotFound

# from src.shell.command.memcomb import MemComb
# from src.shell.command.memuaf import MemUAF
# from src.shell.command.couple import Couple
from src.shell.command.checkconfig import CheckConfigCmd
from src.shell.command.make import MakeCmd
from src.shell.command.display import DisplayCmd
from src.shell.command.parsedata import ParseDataCmd
from src.shell.command.accuracy import AccuracyCmd
from src.shell.command.launch import LaunchCmd
from src.shell.command.memcomb import MemCombCmd
from src.shell.command.couple import CoupleCmd

from src.shell.pin.pintool import Pintool
# from src.shell.test import ScatTest

class ScatShell(Cmd):

    prompt = 'scat > '

    def __init__(self, config_path="config/config.yaml"):
        conf = Confiture("config/templates/general.yaml")
        # Parse configuration file and get result
        self.__config = conf.check_and_get(config_path)
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
            # Check potential extra argument
            if "prev_step" in self.__config["pintool"][pintool].keys():
                prev_step = self.__config["pintool"][pintool]["prev_step"]
            else:
                prev_step = None
            # Create pintool object
            pintool_obj = Pintool(
                                    name=pintool,
                                    src_path=src,
                                    obj_path=obj,
                                    pinconf=self.__config["pin"],
                                    # stdout=self.out,
                                    # stderr=self.out,
                                    log_dir=self.__logdir,
                                    prev_step=prev_step,
                                )
            self.__pintools[pintool] = pintool_obj

        # # Create a test object
        # # Testing options
        # kwargs = dict()
        # # kwargs["log"] = self.out
        # kwargs["clang"] = self.config["clang"]
        # self.test = ScatTest(**kwargs)

        # Enable commands
        self.__cmds = dict()
        self.__cmds["checkconfig"] = CheckConfigCmd(
                                                        logdir=self.__logdir, 
                                                        pinpath=self.__config["pin"]["bin"],
                                                    )
        self.__cmds["make"] = MakeCmd(
                                        pintools=self.__pintools,
                                    )

        self.__cmds["display"] = DisplayCmd(
                                        pintools=self.__pintools,
                                        logdir=self.__logdir,
                                    )
        self.__cmds["parsedata"] = ParseDataCmd()
        self.__cmds["accuracy"] = AccuracyCmd(
                                        pintools=self.__pintools,
                                        logdir=self.__logdir,
                                    )
        self.__cmds["launch"] = LaunchCmd(
                                        pintools=self.__pintools,
                                    )
        self.__cmds["couple"] = CoupleCmd(
                                        pintools=self.__pintools,
                                    )
        self.__cmds["memcomb"] = MemCombCmd(
                                        pintools=self.__pintools,
                                    )

        # Link methods to scat shell
        for cmd, obj in self.__cmds.items():
            setattr(self, "do_" + cmd, obj.run)
            setattr(self, "help_" + cmd, obj.help)
            setattr(self, "complete_" + cmd, obj.complete)

        # Init shell
        Cmd.__init__(self, completekey='tab')

    def emptyline(self):
        pass
 
#     #========== LOG functions ==========#
# 
# #     def out(self, msg, verbose=True, crlf=True):
# #         """
# #             Print message on standard input, with formatting.
# # 
# #             @param msg  message to print
# # 
# #         """
# #         if verbose:
# #             sys.stdout.write("[*] " + msg)
# #             if crlf:
# #                 sys.stdout.write("\n")
# 
# 
# 
#     #========== time of execution ==========#
# 
#     def help_time(self):
#         print(self.do_display.__doc__.replace("\n", " "))
# 
#     def complete_time(self, text, line, begidx, endidx):
#         pgm_inf  = self.__get_pgm_list()
#         for p, inf in pgm_inf.items():
#             if line.find(p) >= 0:
#                 return [i for i in inf if i.startswith(text)]
#         return [pgm for pgm, inf in pgm_inf.items() if pgm.startswith(text)]
# 
#     def do_time(self, s):
#         """
#             Display the time of execution of a given inference
# 
#         """
#         try:
#             pgm, pintool = self.__get_pgm_and_inf(s)
#         except ValueError:
#             return
#         except KeyError:
#             #TODO explicit message (w/ pintool and binary details)
#             self.stderr("Pintool error")
#             return
# 
#         pintool.get_analysis(pgm).time()
#         
# 
#     #========== testing ==========#
# 
#     def help_test(self):
#         print(self.do_test.__doc__.replace("\n", ""))
# 
#     def complete_test(self, text, line, begidx, endidx):
#         # TODO
#         pass
# 
#     def do_test(self, s):
#         # TODO documentation
#         # TODO check that config is specified in config file (+template)
#         # Check CLANG configuration
#         conf = Confiture("config/templates/clang.yaml")
#         conf.check("config/config.yaml")
#         if s == "":
#             self.stderr("No configuration file provided for test -- aborting")
#             return
#         elif not s.endswith(".yaml"):
#             s += ".yaml"
#         self.test.test_all(self.__pintools["arity"], self.__pintools["type"], s)
# 
# 
#     #========== mismatch ==========#
# 
#     def help_mismatch(self):
#         print(self.do_accuracy.__doc__.replace("\n", ""))
# 
#     def complete_mismatch(self, text, line, begidx, endidx):
#         return self.complete_display(text, line, begidx, endidx)
# 
#     def do_mismatch(self, s):
#         """
#             Displays all mismatch for a given program,
#             by comparison with binary and source code.
# 
#         """
#         try:
#             pgm, pintool = self.__get_pgm_and_inf(s)
#         except ValueError:
#             return
#         except KeyError:
#             #TODO explicit message (w/ pintool and binary details)
#             self.stderr("Pintool error")
# 
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
# 
#     #========== new pintool ==========#
# 
# 
#     #========== ALLOC RETRIEVING ==========
# 
#     def do_memcomb(self, s):
# 
#     #========== DETECT SIMPLIFIED UAF ==========
# 
#     def do_memuaf(self, s):
#         pgm = s.split(" ")[0]
#         # Get log file from last block inference
#         if "memblock" not in self.__pintools.keys():
#             self.stderr("you must run memblock inference first")
#             return
#         proto_logfile = self.__pintools["memblock"].get_logfile(pgm, prev=True)
#         mem_logfile = self.__pintools["memblock"].get_logfile(pgm, prev=False)
#         if len(s.split(" ")) > 1:
#             ALLOC, FREE = s.split(" ")[1:3]
#         else:
#             ALLOC, FREE = MemComb(mem_logfile, proto_logfile, self.out).run(wrappers=False)
#         MemUAF(mem_logfile, self.out).run(ALLOC, FREE)
# 
