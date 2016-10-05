# -*- coding: utf-8 -*-

from src.shell.command.i_command import ICommand
from src.shell.parser.type import TypeLogParser
from src.shell.parser.memblock import MemblockParser
from src.shell.command.memcomb import MemComb, AddrTable

class MemUAF(ICommand):
    """
        Detect simplified use-after-free

    """

    def __init__(self, mem_log_file, log):
        super(MemUAF, self).__init__()
        self.__parser = MemblockParser(mem_log_file)
        self.log = log

    def run(self, ALLOC, FREE):
        print ALLOC, FREE
        B_ALLOCATED = AddrTable()
        B_FREED = AddrTable()
        TO_FREE = list()
        mem_size = list()
        for block in self.__parser.get():
            if block.val == 0:
                continue
            if block.id == ALLOC and block.is_out() and block.is_addr():
                if B_ALLOCATED.contains(block.val):
                    self.log("ERROR: block {0} allocated twice".format(block.val))
                else:
                    if B_FREED.contains(block.val):
                        B_FREED.remove(block.val)
                    B_ALLOCATED.add(block.val)
            elif block.id == FREE and block.is_in() and block.is_addr():
                if B_FREED.contains(block.val):
                    self.log("ERROR: block {0} was freed twice".format(block.val))
                elif not B_ALLOCATED.contains(block.val):
                    self.log("ERROR: block {0} was freed but never allocated".format(block.val))
                else:
                    TO_FREE.append(block.val)
            elif block.id == FREE and block.is_out():
                for val in TO_FREE:
                    if not B_ALLOCATED.contains(val):
                        self.log("ERROR: block {0} was freed twice".format(block.val))

                    B_ALLOCATED.remove(val)
                    B_FREED.add(val)
                TO_FREE = list()
            elif block.is_addr() and block.is_in() and not B_ALLOCATED.contains(block.val):
                if B_FREED.contains(block.val):
                    self.log("ERROR: block {0} was used in {1} after being freed".format(block.val, block.id))
        return 

