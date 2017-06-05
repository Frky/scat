#-*- coding: utf-8 -*-

import json 

from src.shell.command.i_command import ICommand
from src.shell.parser.memalloc import MemallocParser

class ExportJSON(ICommand):
    """
        Retrieve couples

    """

    def __init__(self, logfile, log, pgm):
        super(ExportJSON, self).__init__()
        self.__parser = MemallocParser(logfile)
        self.log = log

    def run(self, output):
        data = dict()
        for block in self.__parser.get():
            if block.val < 1000 or block.id == 0:
                continue
            data.setdefault(block.val, list())
            data[block.val].append(block.to_dict())
        #TODO check existence of file 
        # Ask for confirmation before overriding
        with open(output, "w") as f:
            f.write(json.dumps(data))

