#-*- coding: utf-8 -*-

from src.shell.command.i_command import ICommand
from src.shell.parser.block_trace import BlockTraceParser

class MemComb(ICommand):
    """
        Retrieve allocators

    """

    def __init__(self, log_file, log):
        super(MemComb, self).__init__()
        self.__parser = BlockTraceParser(log_file)
        self.log = log

    def run(self):
        func = dict()
        addr_seen = list()
        for i in xrange(BlockTraceParser.DATA_SIZE):
            addr_seen.append(list())
        for io, typ, val, name, counter in self.__parser.get():
            print io, typ, val, name, counter
            if name not in func.keys():
                func[name] = [0]
            if io == "out" and typ == "addr":
                func[name][0] += 1
                # key = val % BlockTraceParser.DATA_SIZE
                # if val not in addr_seen[key]:
                #     func[name][0] += 1
            #key = val % BlockTraceParser.DATA_SIZE
            #if val not in addr_seen[key]:
            #    addr_seen[key].append(val)
        alloc_s = sorted(func.items(), key=lambda a:a[1][0])[-1]
        for a in alloc_s[:100]:
            print a
        return
        self.log("allocator found - {0}".format(alloc_s))
        return
        addr_alloc = list()
        for i in xrange(BlockTraceParser.DATA_SIZE):
            addr_alloc.append(dict())
        for io, typ, val, name, counter in self.__parser.get():
            key = val % BlockTraceParser.DATA_SIZE
            if val not in addr_alloc[key].keys():
                addr_alloc[key][val] = list()
            if io == "out" and typ == "addr" and name == alloc_s:
                key = val % BlockTraceParser.DATA_SIZE
                addr_alloc[key][val].append(name)
            elif io == "in" and typ == "addr" and name != alloc_s:
                addr_alloc[key][val].append(name)
        func = dict()
        for val in addr_alloc:
            for addr, call in val.items():
                if len(call) == 0 or call[0] != alloc_s:
                    continue
                while call.count(alloc_s) > 0:
                    if call.index(alloc_s) == 0:
                        call.pop(0)
                    else:
                        free = call[call.index(alloc_s) - 1]
                        if free not in func.keys():
                            func[free] = 0
                        func[free] += 1
                        call = call[call.index(alloc_s)+1:]
        free_s = sorted(func.items(), key=lambda a:a[1])[-1][0]
        self.log("liberator found - {0}".format(free_s))
        return # alloc_s, free_s

