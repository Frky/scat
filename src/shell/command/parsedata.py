#-*- coding: utf-8 -*-

import os 
from confiture import Confiture, ConfigFileError

from .i_command import ICommand
from src.shell.data.data import Data

class ParseDataCmd(ICommand):
    """
        usage: parsedata program src

        Non-optional arguments:
            program: program of which you want to analyse source code
            src: directory of the program's source code
    """

    def __init__(self, *args, **kwargs):
        super(ParseDataCmd, self).__init__(*args, **kwargs)
        return

    def run(self, s, *args, **kwargs):
        # TODO check number of args
        # TODO completion on args
        split = s.split(" ")
        if len(split) == 1:
            binary, srcdir = split[0], None
        else:
            binary, srcdir = split

        pgm = os.path.basename(binary)

        # Check CLANG configuration
        config = Confiture("config/templates/clang.yaml").check_and_get("config/config.yaml") 
        # Create a parser object
        data = Data(config["clang"]["data-path"], pgm)
        data.parse(binary, config["clang"]["lib-path"], srcdir)
        data.dump()
