#-*- coding: utf-8 -*-

import traceback

from .i_command import ICommand

from src.shell.memory.memcomb import MemComb
from src.shell.utils import *

class MemCombCmd(ICommand):
    """
        usage: memcomb program [OPTION]

        Optional arguments:
            --lib: search also in the library used by the binary
            --ignore=<keyword1,keyword2,..>: ignore all the memory blocks
                containig at least one of the keywords
            --libmatch=<keyword1,keyword2,..>: keep only the memory blocks
                that matches at least one of the keywords
            --couple: enable the filtering of free candidate. Only the functions
                coupled with the infered allocator will be evaluated.
                Requires `couple program` to be run before.

        memcomb must be used only after using succesfully `launch type` and
        `launch memalloc`
    """

    def __init__(self, pintools, log_manager, *args, **kwargs):
        self.__pintools = pintools
        self.__log_manager = log_manager
        self.__logdir = log_manager._logdir
        super(MemCombCmd, self).__init__(*args, **kwargs)
        return

    def run(self, s, *args, **kwargs):
        # Get log file from last block inference
        if "memalloc" not in self.__pintools.keys():
            self.stderr("you must run memalloc inference first")
            return

        if len(s) == 0:
            self.stderr("You must give at least one argument (pgm)")
            return


        s = s.split()
        cli_ignore = None
        cli_libmatch = None
        libraries = False
        couple = False
        for i in s:
            if "--ignore=" in i:
                cli_ignore = i.replace("--ignore=","").split(",")
            if "--libmatch=" in i:
                cli_libmatch = i.replace("--libmatch=","").split(",")
            if i == "--lib":
                libraries = True
            if i == "--couple":
                couple = True


        try:
            proto_logfile = self.__log_manager.get_log("type", s[0])
            mem_logfile = self.__log_manager.get_log("memalloc", s[0])
            if couple:
                coupleres_log_file = self.__log_manager.get_log("coupleres", s[0])
                print(coupleres_log_file)
            else:
                coupleres_log_file = None
        except IOError:
            self.stderr("Logs for binary \"{}\" not found".format(s[0]))
            return

        try:
            MemComb(mem_logfile,
                    proto_logfile,
                    self.stdout,
                    s[0],
                    outfile=self.__log_manager.gen_log("memcomb", s[0]),
                    cli_ignore=cli_ignore,
                    cli_libmatch=cli_libmatch,
                    coupleres_log_file=coupleres_log_file
                    ).run(libraries=libraries)
        except Exception as e:
            traceback.print_exc()
            raise e

    def complete(self, text, line, begidx, endidx):
        return complete_pgm_pintool(text, line, self.__logdir, complete_pintool=False)
