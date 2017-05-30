#-*- coding: utf-8 -*-

from .i_command import ICommand

from src.shell.couple.couple import Couple

class CoupleCmd(ICommand):
    """
        usage: couple program

        Non-optional argument:
            program: program you want to compute couples on

        couple must be used only after using successfully `launch couple`
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
        if len(s) == 0:
            self.stderr("You must give at least one argument (pgm)")
            return

        s = s.split()

        try:
            logfile = self.__pintools["couple"].get_logfile(s[0], prev=False)
        except IOError:
            self.stderr("Logs for binary \"{}\" not found".format(s[0]))
            return
        Couple(logfile, s[0], self.stdout).run()
