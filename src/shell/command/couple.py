#-*- coding: utf-8 -*-

from .i_command import ICommand

from src.shell.couple.couple import Couple
from src.shell.utils import *

class CoupleCmd(ICommand):
    """
        usage: couple program [--min_rho=<value>]

        Optional argument:
            --min_rho=<value>: minimum value of the rho coefficient for two
                function to be considered coupled

        Non-optional argument:
            program: program you want to compute couples on

        couple must be used only after using successfully `launch couple`
    """

    def __init__(self, pintools, log_manager, *args, **kwargs):
        self.__pintools = pintools
        self.__log_manager = log_manager
        super(CoupleCmd, self).__init__(*args, **kwargs)
        return

    def run(self, s, *args, **kwargs):
        # Get log file from last block inference
        if "couple" not in self.__pintools.keys():
            self.stderr("you must run memblock inference first")
            return
        if len(s) == 0:
            self.stderr("You must give at least one argument (pgm)")
            return

        s = s.split()
        min_rho = 0.5
        for i in s:
            if "--min_rho" in i:
                min_rho = float(i.replace("--min_rho=",""))

        try:
            couple_logfile = self.__log_manager.get_log("couple", s[0])
        except IOError:
            self.stderr("Logs for binary \"{}\" not found".format(s[0]))
            return
        Couple(
                s[0],
                infile=couple_logfile,
                outfile=self.__log_manager.gen_log("coupleres", s[0])
                ).run(min_rho=min_rho)

    def complete(self, text, line, begidx, endidx):
        return complete_pgm_pintool(text, line, self.__log_manager.__logdir, complete_pintool=False)
