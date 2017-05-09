#-*- coding: utf-8 -*-

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
        s = s.split(" ")
        libraries = len(s) > 1 and s[1] == "--lib"
        proto_logfile = self.__pintools["memalloc"].get_logfile(s[0], prev=True)
        mem_logfile = self.__pintools["memalloc"].get_logfile(s[0], prev=False)
        MemComb(mem_logfile, proto_logfile, self.out, s[0]).run(libraries=libraries)
