#-*- coding: utf-8 -*-

import traceback

from .i_command import ICommand

from src.shell.memory.memcomb import MemComb
from src.shell.utils import complete_bin, complete_path, checkpath

class MemCombCmd(ICommand):
    """

    """

    def __init__(self, pintools, *args, **kwargs):
        self.__pintools = pintools
        super(MemCombCmd, self).__init__(*args, **kwargs)
        return

    def run(self, s, *args, **kwargs):
        # Get log file from last block inference
        if "memalloc" not in self.__pintools.keys():
            self.stderr("you must run memalloc inference first")
            return
        libraries = len(s) > 1 and s[1] == "--lib"

        if len(s) == 0:
            self.stderr("You must give at least one argument (pgm)")
            return


        s = s.split()

        try:
            proto_logfile = self.__pintools["type"].get_logfile(s[0], prev=False)
            mem_logfile = self.__pintools["memalloc"].get_logfile(s[0], prev=False)
        except IOError:
            self.stderr("Logs for binary \"{}\" not found".format(s[0]))
            return

        try:
            MemComb(mem_logfile, proto_logfile, self.stdout, s[0]).run(libraries=libraries)
        except Exception as e:
            traceback.print_exc()
            raise e
