#-*- coding: utf-8 -*-

from .i_command import ICommand

from src.shell.couple.couple import Couple

class CoupleCmd(ICommand):
    """
    """

    def __init__(self, pintools, *args, **kwargs):
        self.__pintools = pintools
        super(CoupleCmd, self).__init__(*args, **kwargs)
        return

    def run(self, s, *args, **kwargs):
        # Get log file from last block inference
        if "couple" not in self.__pintools.keys():
            self.stderr("you must run memblock inference first")
            return
        logfile = self.__pintools["couple"].get_logfile(s, prev=False)
        Couple(logfile, self.stdout).run()

