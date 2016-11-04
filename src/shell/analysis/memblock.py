#-*- coding: utf-8 -*-

from datetime import datetime
import re
from analysis import Analysis
from src.shell.parser.memblock import MemblockParser

class MemblockAnalysis(Analysis):

    def __init__(self, pgm, logfile, data=None):
        Analysis.__init__(self, pgm, logfile)

        self.data = data
        if data == None:
            self.protos = None
        else:
            self.protos = data.protos

    def parse_log(self):
        self.log = MemblockParser(self.logfile)

    def print_general_info(self):
        if self.data is None:
            Analysis.print_general_info(self)
        else:
            Analysis.print_general_info_with_data(self, self.data)

    def display(self):
        for function, args in self.log.get():
            img, imgaddr, fn = function.split(":")
            imgaddr = int(imgaddr)
            print(self.args_str(img, imgaddr, fn, args))
        print("")
        self.print_general_info()

