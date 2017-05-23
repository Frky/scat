#-*- coding: utf-8 -*-

from src.shell.utils import get_pgm_and_inf, get_pgm_list
from .i_command import ICommand

class DisplayCmd(ICommand):
    """
        usage: display [program (arity | type)]

            program: program you have previously analysed

        Display results of inference
        If no argument is given, display all the eligible programs.
    """

    def __init__(self, pintools, logdir, *args, **kwargs):
        self.__pintools = pintools
        self.__logdir = logdir
        super(DisplayCmd, self).__init__(*args, **kwargs)
        return

    def run(self, s, *args, **kwargs):
        try:
            pgm, pintool = get_pgm_and_inf(s, self.__pintools, self.__logdir)
        except ValueError:
            return
        except KeyError:
            #TODO explicit message (w/ pintool and binary details)
            self.stderr("Pintool error")
            return

        pintool.get_analysis(pgm).display()

    def complete(self, text, line, begidx, endidx):
        pgm_inf  = get_pgm_list(self.__logdir)
        for p, inf in pgm_inf.items():
            if line.find(p) >= 0:
                return [i for i in inf if i.startswith(text)]
        return [pgm for pgm, inf in pgm_inf.items() if pgm.startswith(text)]
