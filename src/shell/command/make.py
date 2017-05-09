#-*- coding: utf-8 -*-

from .i_command import ICommand

class MakeCmd(ICommand):
    """
        (Re)compile pintools. With no argument, this command recompiles
        every pintool registered.

        You can also specify the pintools you want to compile (e.g. make arity type)

        Options:
            -f: force recompilation
            -d: compile enabling debug messages at execution
            -t: compile enabling trace messages at execution

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
            p.compile(force, debug, trace)

