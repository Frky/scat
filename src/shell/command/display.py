#-*- coding: utf-8 -*-

from src.shell.utils import *
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
            self.stderr("Pintool \"{}\" not found".format(s.split(" ")[1]))
            return

        pintool.get_analysis(pgm).display()

    def complete(self, text, line, begidx, endidx):
        return complete_pgm_pintool(text, line, self.__logdir)
