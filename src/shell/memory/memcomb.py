# -*- coding: utf-8 -*-

from numpy import mean 
from random import randint

from src.shell.callstack import CallStack
from src.shell.parser.type import TypeLogParser
from src.shell.parser.memalloc import MemallocParser
from .addrtable import AddrTable
from .memory import Memory

class MemComb(object):
    """
        Retrieve allocators

    """

    def __init__(self, mem_log_file, type_log_file, log, pgm):
        self.__parser = MemallocParser(mem_log_file)
        self.__protos = TypeLogParser(type_log_file)
        self.log = log
        self.__pgm = pgm
        super(MemComb, self).__init__()

    def __compute_callers(self):
        call_stack = CallStack(self.__pgm)
        callers = dict()
        for block in self.__parser.get():
            # if block.is_in():
            #     if not libraries and block.is_from_main():
            #         callers.setdefault(block.id, 0)
            #         callers[block.id] += 1
            if block.is_in():
                caller = call_stack.top_id()
                call_stack.push(block)
            else:
                call_stack.pop()
                caller = call_stack.top_id()
            callers.setdefault(block.id, list())
            if caller is not None and caller not in callers[block.id]:
                callers[block.id].append(caller)
        for k, v in callers.items():
            if ".plt.got" in [x.split(":")[-1] for x in v]:
                callers[k] = 3
            else:
                callers[k] = len(v)
        return callers

    def __alloc(self, nb_callers, libraries):
        """
            Try to retrieve the top-level allocator

        """
        # Number of new addresses outputted by each function
        nb_new_addr = dict()
        # Addresses seen so far
        addr_seen = AddrTable()
        # Call stack
        call_stack = CallStack(self.__pgm)
        # For each block of data in the log file
        for block in self.__parser.get():
            # IN PARAMETER
            if block.is_in():
                call_stack.push(block)
            # OUT PARAMETER
            else:
                call_stack.expect(block.id, block.date)
                # if block.id == "libc.so.6:529408:" and call_stack.depth <= 1:
                #    print call_stack.stack
                if call_stack.depth > 0 and not libraries:
                    call_stack.pop()
                    continue
                if block.is_addr():
                    if block.id not in nb_new_addr.keys():
                        nb_new_addr[block.id] = [0, 0]
                    if not addr_seen.contains(block.val): 
                        if not libraries or nb_callers[block.id] > 1:
                            if 'ADDR' not in self.__protos.get_proto(block.id)[1:]:
                                nb_new_addr[block.id][0] += 1
                                addr_seen.add(block.val)
                    nb_new_addr[block.id][1] += 1
                call_stack.pop()
        ll = sorted(nb_new_addr.items(), key=lambda a: -a[1][0])
        for f in ll:
            proto = self.__protos.get_proto(f[0])
            if 'ADDR' not in proto[1:]:
                return f[0]
        return None

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
        call_stack = CallStack(self.__pgm)
        # For each block of data in the log file
        for block in self.__parser.get():
            # IN PARAMETER
            if block.is_in():
                call_stack.push(block)
            # OUT PARAMETER
            else:
                call_stack.expect(block.id, block.date)
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

    def __free(self, ALLOC, libraries):
        nb_alloc = 0
        # Number of new addresses outputted by each function
        nb_new_addr = dict()
        # Addresses seen so far
        addr_alloc = AddrTable(dic=True)
        # Call stack
        call_stack = CallStack(self.__pgm)
        # Number of calls
        nb_calls = dict()
        for block in self.__parser.get():
            if block.is_out():
                nb_calls.setdefault(block.id, 0)
                nb_calls[block.id] += 1
            if block.id.split(":")[0] != self.__pgm and not libraries:
                continue
            if block.is_addr():
                if block.is_in() and block.id != ALLOC:
                    if not addr_alloc.contains(block.val):
                        addr_alloc.add(block.val)
                    block_id = "{0}|{1}".format(block.id, block.pos)
                    addr_alloc.add_dic(block.val, block_id)
                elif block.is_out() and block.id == ALLOC:
                    if not addr_alloc.contains(block.val):
                        addr_alloc.add(block.val)
                    addr_alloc.add_dic(block.val, block.id)
        for addr, call in addr_alloc.items():
            if len(call) == 0 or call.count(ALLOC) == 0:
                continue
            nb_alloc += call.count(ALLOC)
            candidates = map(lambda a: a[-1], list_split(call, ALLOC))
            for free in candidates:
                # while call.count(ALLOC) > 0:
                # if call.index(ALLOC) == 0:
                #    call.pop(0)
                #    if len(call) > 0:
                #        free = call[-1]
                #    else: 
                #        continue
                # else:
                #     free = call[call.index(ALLOC) - 1]
                #     call = call[call.index(ALLOC)+1:]
                if free not in nb_new_addr.keys():
                    nb_new_addr[free] = 0
                nb_new_addr[free] += 1 
                # call.count(free)
        free = sorted(nb_new_addr.items(), key=lambda a:-a[1])
        free = free[0][0].split("|")
        return free[0], free[1]

    def __compute_blocks(self, ALLOC, FREE, POS):
        mem = Memory(debug=False)
        for block in self.__parser.get():
            if block.id == ALLOC and block.is_out():
                mem.alloc(block.val, 1)
            elif block.id == FREE and block.is_in():
                mem.free(block.val)

        # size_stack = [(0, -1)]
        # for block in self.__parser.get():
        #     if block.id.split(":")[0] != self.__pgm:
        #         continue
        #     if block.id == ALLOC:
        #         # if block.is_in() and block.is_num() and size_stack[-1][1] != block.date:
        #         #     size_stack.append((block.val, block.date))
        #         if block.is_out():
        #             # if len(size_stack) <= 1:
        #             #     raise Exception("ALLOC stack inconsistancy at date {0}".format(block.date))
        #             size, date = size_stack.pop(-1)
        #             mem.alloc(block.val, size)
        #     else:
        #         is_free = False
        #         for free, pos in zip(FREE, POS):
        #             if block.id == free and block.pos == int(pos):
        #                 if block.is_in():
        #                     mem.free(block.val)
        #                 is_free = True
        #         if not is_free and block.is_addr() and block.is_in() and not mem.is_allocated(block.val):
        #             print "UAF", block.id, block.val, block.date

        print "[errors] ALLOC: {0} | FREE: {1}".format(mem.errors[0], mem.errors[1])
        print "[allocs] CURR: {0} | TOTAL: {1}".format(*mem.allocated)
        print "[nbcall] ALLOC: {0} | FREE: {1}".format(*mem.nb_calls)
        return

    def run(self, libraries=False, wrappers=True):
        if libraries:
            nb_callers = self.__compute_callers()
        else:
            nb_callers = None
        ALLOC = self.__alloc(nb_callers, libraries)
        self.log("allocator found - {0}".format(ALLOC))
        FREE, POS = self.__free(ALLOC, libraries)
        self.log("liberator found - {0}".format(FREE))
        self.log("checking consistancy of blocks...")
        self.compute_blocks(ALLOC, FREE, POS)
        # if wrappers:
        #     # Detecting suballocators
        #     SUBALLOC = self.__wrappers(ALLOC)
        return ALLOC, FREE

def list_split(l, e):
    res = list()
    curr = list()
    if l.count(e) == 0:
        return l
    for a in l[l.index(e):]:
        if a != e:
            curr.append(a)
        elif len(curr) > 0:
            res.append(curr)
            curr = list()
    if len(curr) > 0:
        res.append(curr)
    return res

