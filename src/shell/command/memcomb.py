# -*- coding: utf-8 -*-

from src.shell.command.i_command import ICommand
from src.shell.parser.type import TypeLogParser
from src.shell.parser.block_trace import BlockTraceParser

from numpy import mean 

class MemComb(ICommand):
    """
        Retrieve allocators

    """

    def __init__(self, mem_log_file, type_log_file, log):
        super(MemComb, self).__init__()
        self.__parser = BlockTraceParser(mem_log_file)
        self.__protos = TypeLogParser(type_log_file)
        self.log = log

    def __alloc(self):
        """
            Try to retrieve the top-level allocator

        """
        return "libc.so.6:529408"
        # Number of new addresses outputted by each function
        nb_new_addr = dict()
        # Addresses seen so far
        addr_seen = AddrTable()
        # Call stack
        call_stack = CallStack()
        # For each block of data in the log file
        for block in self.__parser.get():
            # IN PARAMETER
            if block.is_in():
                call_stack.push(block)
            # OUT PARAMETER
            else:
                call_stack.expect(block.id)
                if block.is_addr():
                    new = True
                    if addr_seen.contains(block.val):
                        new = False
                    if new:
                        nb_new_addr[block.id][0] += 1
                        # in_num = filter(lambda a: a.is_num(), call_stack.top())
                        # if len(in_num) == 0:
                        #     size = 0
                        # else:
                        #     size = min(map(lambda a: a.val, in_num))
                        addr_seen.add(block.val)
                        # [key].append((block.val, block.date, size))
                    nb_new_addr[block.id][1] += 1
                call_stack.pop()
        return max(nb_new_addr.items(), key=lambda a: a[1][0])[0]
        alloc_s = sorted(nb_new_addr.items(), key=lambda a:-(a[1][0]))
        ALLOC = alloc_s[0][0]

    def __wrappers(self, ALLOC):
        # Number of new addresses outputted by each function
        wrappers = dict()
        # Last output value
        last_addr = None
        #
        prev = None
        # TTL
        ttl = 0
        # Call stack
        call_stack = CallStack()
        # For each block of data in the log file
        for block in self.__parser.get():
            # IN PARAMETER
            if block.is_in():
                call_stack.push(block)
            # OUT PARAMETER
            else:
                call_stack.expect(block.id)
                if block.is_addr():
                    if block.id in wrappers.keys():
                        wrappers[block.id][1] += 1
                    if block.id == ALLOC:
                        last_addr = block.val
                        depth = 1
                        prev = ALLOC
                    elif last_addr is not None:
                        if block.val == last_addr:
                            if block.id not in wrappers.keys():
                                wrappers[block.id] = [0, 1, list(), list()]
                            wrappers[block.id][0] += 1
                            wrappers[block.id][2].append(depth)
                            wrappers[block.id][3].append(prev)
                            depth += 1
                            prev = block.id
                        # else:
                        #     last_addr = None
                        #     ttl = 0
                        #     prev = None
                call_stack.pop()
                if call_stack.is_empty():
                    last_addr = None
        wrappers = map(lambda a: (a[0], float(a[1][0])/float(a[1][1]), mean(a[1][2]), max(set(a[1][3]), key=a[1][3].count)), sorted(wrappers.items(), key=lambda a: a[1][0]))
        wrappers = sorted(filter(lambda a: a[1] > 0.5, wrappers), key=lambda a:a[2])
        WTREE = Wrapper(ALLOC, 1)
        for wrapper in wrappers:
            wrap = WTREE.get(wrapper[3], wrapper[2] - 1)
            if wrap is not None:
                wrap.add_child(Wrapper(wrapper[0], int(wrapper[1])))
            else:
                print "Elaged: {0}".format(wrapper[0])
        print WTREE.to_str(0)

    def __free(self, ALLOC):
        return "libc.so.6:522592"
        # Number of new addresses outputted by each function
        nb_new_addr = dict()
        # Addresses seen so far
        addr_alloc = AddrTable(dic=True)
        # Call stack
        call_stack = CallStack()
        for block in self.__parser.get():
            if not addr_alloc.contains(block.val):
                addr_alloc.add(block.val)
            if block.is_in() and block.id != ALLOC:
                addr_alloc.add_dic(block.val, block.id)
            elif block.is_out() and block.id == ALLOC:
                addr_alloc.add_dic(block.val, block.id)
        for addr, call in addr_alloc.items():
            if len(call) == 0 or call[0] != ALLOC:
                continue
            while call.count(ALLOC) > 0:
                if call.index(ALLOC) == 0:
                    call.pop(0)
                else:
                    free = call[call.index(ALLOC) - 1]
                    if free not in nb_new_addr.keys():
                        nb_new_addr[free] = 0
                    nb_new_addr[free] += 1
                    call = call[call.index(ALLOC)+1:]
        return max(nb_new_addr.items(), key=lambda a:a[1])[0]

    def run(self):
        ALLOC = self.__alloc()
        self.log("allocator found - {0}".format(ALLOC))
        FREE = self.__free(ALLOC)
        self.log("liberator found - {0}".format(FREE))
        # Detecting suballocators
        SUBALLOC = self.__wrappers(ALLOC)
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

    def is_empty(self):
        return len(self.__stack) == 0

    def items(self):
        for blocks in reversed(self.__stack):
            yield blocks


class AddrTable(object):

    TABLE_SIZE = 1000000

    def __init__(self, dic=False):
        self.__addr = list()
        self.__dic = dic
        for i in xrange(AddrTable.TABLE_SIZE):
            if self.__dic:
                self.__addr.append(dict())
            else:
                self.__addr.append(list())
        self.__curr_key = None
        self.__curr_addr = None

    def contains(self, addr):
        key = addr % AddrTable.TABLE_SIZE
        if self.__dic:
            return addr in self.__addr[key].keys()
        else:
            return addr in self.__addr[key]

    def add(self, addr):
        key = addr % AddrTable.TABLE_SIZE
        if self.__dic:
            self.__addr[key][addr] = list()
        else:
            self.__addr[key].append(addr)

    def add_dic(self, addr, fid):
        if not self.__dic:
            raise Exception
        key = addr % AddrTable.TABLE_SIZE
        self.__addr[key][addr].append(fid)

    def items(self):
        for key in self.__addr:
            if self.__dic:
                for addr, call in key.items():
                    yield addr, call
            else:
                for addr in key:
                    yield addr


class Wrapper(object):

    def __init__(self, wid, ratio):
        self.__id = wid
        self.__ratio = ratio
        self.__next = list()

    @property
    def id(self):
        return self.__id

    @property
    def ratio(self):
        return self.__ratio

    @property
    def next(self):
        return self.__next

    def add_child(self, wrap):
        self.__next.append(wrap)

    def get(self, wid, d):
        if d == 0:
            if wid == self.id:
                return self
            else:
                return None
        for n in self.next:
            nw = n.get(wid, d - 1)
            if nw:
                return nw
        return None

    def to_str(self, d):
        s = ""
        for i in xrange(d):
            s += "**"
        s += "| " + str(self.id) + "\n"
        for n in self.next:
            s += n.to_str(d + 1)
        return s

