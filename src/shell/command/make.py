#-*- coding: utf-8 -*-

from .i_command import ICommand
from src.shell.utils import *

class MakeCmd(ICommand):
    """
        usage: make [OPTION] [pintool1 pintool2 ..]

        Optional arguments:
            pintool1, pintool2, etc.: list of pintools to compile
            -t, --trace: compile enabling trace messages at execution
            -d, --debug: compile enabling debug messages at execution
            -f, --force, -B: force recompilation

        If there is no pintool specified, compile all the pintools.
    """

    def __init__(self, pintools, *args, **kwargs):
        self.__pintools = pintools
        super(MakeCmd, self).__init__(*args, **kwargs)
        return

    def run(self, s, *args, **kwargs):
        force = False
        debug = False
        trace = False

        if s != "":
            args = s.split(" ")
        else:
            args = list()

        to_compile = list()

        for arg in args:
            if arg == '-f' or arg == '--force' or arg == '-B':
                force = True
                continue
            if arg == '-d' or arg == '--debug':
                debug = True
                continue
            if arg == '-t' or arg == '--trace':
                trace = True
                continue
            try:
                p = self.__pintools[arg]
            except KeyError:
                self.stderr("pintool {0} is unknown".format(arg))
                return
            to_compile.append(p)

        # If no pintool given in argument, compile everything
        if len(to_compile) == 0:
            to_compile = self.__pintools.values()

        # Compile pintools
        for p in to_compile:
            p.compile(force, debug, trace, verbose=True)
