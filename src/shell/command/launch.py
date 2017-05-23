#-*- coding: utf-8 -*-

from .i_command import ICommand
from src.shell.utils import complete_bin, complete_path, checkpath

class LaunchCmd(ICommand):
    """
        usage: launch [OPTION?] pintool program [args]

        Non-optional arguments:
            pintool: pintool you want to use
            program: program you want to analyse


        Optional arguments:
            args: arguments of the program analysed
            -t, --trace:  <TODO>
            -d, --debug:  <TODO>
            -f, --force, -B:  <TODO>
            -r, --release:  <TODO>
    """

    def __init__(self, pintools, *args, **kwargs):
        self.__pintools = pintools
        super(LaunchCmd, self).__init__(*args, **kwargs)
        return

    def run(self, s, *args, **kwargs):
        split = s.split(" ")
        index = 0

        release = False
        force = False
        debug = False
        trace = False
        while index < len(split) and split[index].startswith("-"):
            arg = split[index]
            if arg == '-f' or arg == '--force' or arg == '-B':
                force = True
            elif arg == '-r' or arg == '--release':
                release = True
            elif arg == '-d' or arg == '--debug':
                debug = True
            elif arg == '-t' or arg == '--trace':
                trace = True
            index += 1

        inf = split[index]
        index += 1

        if inf not in self.__pintools.keys():
            self.stderr('Unknown inference "{}"'.format(inf))
            return

        p = self.__pintools[inf]
        if release or debug or trace:
            if not p.compile(force, debug, trace):
                return

        #TODO check config

        # Parse command into binary + args
        args =  list()
        for i, arg in enumerate(split[index:]):
            if arg[0] == "\"":
                arg = arg[1:]
            if arg[-1] == "\"":
                arg = arg[:-1]
            if i == 0:
                binary = arg
            else:
                args.append(arg)

        # Check the binary (exists? is executable?)
        try:
            checkpath(binary, isdir=False, isexec=True)
        except ValueError:
            return
        except UnboundLocalError:
            self.stderr('Missing binary program name as argument')
            return

        # Run inference
        self.stdout("Launching {0} inference on {1}".format(p, binary))
        p.launch(binary, args)

    def complete(self, text, line, begidx, endidx):
        if len(line.split(" ")) < 3:
            return filter(
                            lambda x: x.startswith(text),
                            map(lambda x: str(x), self.__pintools),
                        )
        elif len(line.split(" ")) < 4:
            return complete_bin(text, line, begidx, endidx)
        else:
            return  complete_path(text, line, begidx, endidx)
