#-*- coding: utf-8 -*-

from .i_command import ICommand
from src.shell.utils import *
import os

class LaunchCmd(ICommand):
    """
        usage: launch [OPTION?] (all | pintool) program [args]

        Non-optional arguments:
            pintool: pintool you want to use
            program: program you want to analyse


        Optional arguments:
            args: arguments of the program analysed
            all: Launch all the stable pintool in the right order (arity, type,
                couple)
            OPTION : 
                -t, --trace:  <TODO>
                -d, --debug:  <TODO>
                -f, --force, -B:  <TODO>
                -r, --release:  <TODO>
                --alt_prev: use the alternative previous step as defined in the
                    file to perform the analysis
    """

    def __init__(self, pintools, *args, **kwargs):
        self.__pintools = pintools
        super(LaunchCmd, self).__init__(*args, **kwargs)
        return

    def run(self, s, *args, **kwargs):
        split = s.split()
        index = 0
        self.__options_dict = {"release":False, "force":False, "debug":False,
                "trace":False, "alt_prev":False}
        while index < len(split) and split[index].startswith("-"):
            arg = split[index]
            if arg == '-f' or arg == '--force' or arg == '-B':
                self.__options_dict["force"] = True
            elif arg == '-r' or arg == '--release':
                self.__options_dict["release"] = True
            elif arg == '-d' or arg == '--debug':
                self.__options_dict["debug"] = True
            elif arg == '-t' or arg == '--trace':
                self.__options_dict["trace"] = True
            elif arg == '--alt_prev':
                self.__options_dict["alt_prev"] = True
            index += 1

        if not split:
            self.stderr('Missing arguments')
            return
        inf = split[index]
        index += 1

        if inf == "all":
            for pintool in ["arity", "type", "couple", "memalloc"]:
                continuing = self.launch_pintool(pintool, split[index:], verify=True)
                if not continuing:
                    break
        elif inf not in self.__pintools.keys():
            self.stderr('Unknown inference "{}"'.format(inf))
            return
        else:
            self.launch_pintool(inf, split[index:])


    def complete(self, text, line, begidx, endidx):
        trimmed_line = line.replace('  ', ' ')
        while trimmed_line != line:
            line = line.replace('  ', ' ')
            trimmed_line = trimmed_line.replace('  ', ' ')
        if len(line.split(' ')) < 3:
            return filter(
                            lambda x: x.startswith(text),
                            map(lambda x: str(x), self.__pintools),
                        )
        elif len(line.split(' ')) < 4:
            return complete_bin(text, line, begidx, endidx)
        else:
            return  complete_path(text, line, begidx, endidx)

    def launch_pintool(self, pintool, binary_and_args, verify=False):
        """
        """

        options_dict = self.__options_dict
        if verify and self.check_pintool_prev(pintool, binary_and_args):
            return False
        p = self.__pintools[pintool]

        release = options_dict["release"]
        trace = options_dict["trace"]
        debug = options_dict["debug"]
        force = options_dict["force"]
        alt_prev = options_dict["alt_prev"]
        if release or debug or trace:
            if not p.compile(force, debug, trace):
                return

        #TODO check config

        # Parse command into binary + args
        args =  list()
        for i, arg in enumerate(binary_and_args):
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
            self.stderr("Binary \"{}\" not found".format(binary))
            return
        except UnboundLocalError:
            self.stderr('Missing binary program name as argument')
            return

        # Run inference
        self.stdout("Launching {0} inference on {1}".format(p, binary))
        p.launch(binary, args, alt_prev=alt_prev)       
        return True

    def check_pintool_prev(self, pintool, binary_and_args):
            alt_prev_option = self.__options_dict["alt_prev"]

            prev_step = self.__pintools[pintool].prev_step
            alt_prev_step = self.__pintools[pintool].alt_prev_step

            non_launch_steps = ["coupleres", "memcomb"]

            manual_mode_needed = ((prev_step in non_launch_steps
                and not alt_prev_option)
                or (alt_prev_step in non_launch_steps
                and alt_prev_option))

            if manual_mode_needed:
                self.stderr("You need to manually run \"couple {}\" before"
                        " running \"launch --alt_prev memalloc {}\""
                        .format(os.path.basename(binary_and_args[0]), " ".join(binary_and_args)))

            return manual_mode_needed
