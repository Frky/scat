#-*- coding: utf-8 -*-

from src.shell.command.i_command import ICommand
from src.shell.parser.type import TypeLogParser
from src.shell.parser.block_trace import BlockTraceParser

class MemComb(ICommand):
    """
        Retrieve allocators

    """

    def __init__(self, mem_log_file, type_log_file, log):
        super(MemComb, self).__init__()
        self.__parser = BlockTraceParser(mem_log_file)
        self.__protos = TypeLogParser(type_log_file)
        self.log = log

    def run(self):
        func = dict()
        addr_seen = list()
        for i in xrange(BlockTraceParser.DATA_SIZE):
            addr_seen.append(list())
        call_stack = CallStack()
        for block in self.__parser.get():
            if block.id not in func.keys():
                func[block.id] = [0,1]
            key = block.val % BlockTraceParser.DATA_SIZE
            # IN PARAMETER
            if block.is_in():
                call_stack.push(block)
            # OUT PARAMETER
            else:
                call_stack.expect(block.id)
                if block.is_addr() or True:
                    new = True
                    for val, date, size in addr_seen[key]:
                        # if date > call_stack.top()[1]:
                        #    break
                        if val <= block.val and val + size >= block.val:
                            new = False
                            break
                    if new:
                        func[block.id][0] += 1
                        in_num = filter(lambda a: a.is_num(), call_stack.top())
                        if len(in_num) == 0:
                            size = 0
                        else:
                            size = min(map(lambda a: a.val, in_num))
                        addr_seen[key].append((block.val, block.date, size))
                    func[block.id][1] += 1
                call_stack.pop()
        alloc_s = sorted(func.items(), key=lambda a:-(a[1][0]))
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


class CallStack(object):

    def __init__(self):
        self.__stack = list()

    def pop(self):
        return self.__stack.pop(-1)

    def push(self, block):
        # Only push if fid and date are different from the top
        # of the stack
        if len(self.__stack) > 0 and self.__stack[-1][0].id == block.id and self.__stack[-1][0].date == block.date:
            self.__stack[-1].append(block)
            return
        self.__stack.append([block])
        
    def expect(self, fid):
        if len(self.__stack) == 0 or self.__stack[-1][0].id != fid:
            print "ERROR IN CALL STACK"
            print "Expecting {0}, top is {1}".format(fid, self.top())
            print self.__stack
            raise Exception

    def top(self):
        return self.__stack[-1]

