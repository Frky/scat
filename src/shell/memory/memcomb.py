# -*- coding: utf-8 -*-

from numpy import mean 
from random import randint
from time import time

from src.shell.callstack import CallStack
from src.shell.parser.type import TypeLogParser
from src.shell.parser.memalloc import MemallocParser
from src.shell.utils import list_split
from .addrtable import AddrTable
from .memory import Memory

class MemComb(object):
    """
        Retrieve allocators

    """

    def __init__(self, mem_log_file, type_log_file, log, pgm,
            cli_ignore=None, cli_libmatch=None, coupleres_log_file=None):
        super(MemComb, self).__init__()
        self.__parser = MemallocParser(mem_log_file, cli_ignore, cli_libmatch)
        self.__protos = TypeLogParser(type_log_file)
        if coupleres_log_file is not None:
            self.__couples_file = coupleres_log_file
        else:
            self.__couples_file = None
        self.log = log
        self.__pgm = pgm
        self.__free_candidates = dict()

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

    def __alloc(self, nb_callers, libraries, ignore=None):
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
        ii = 0
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
                    if ignore is not None and block.id in ignore:
                        call_stack.pop()
                        continue
                    if block.id not in nb_new_addr.keys():
                        nb_new_addr[block.id] = [0, 0]
                    if not addr_seen.contains(block.val): 
                        if not libraries or nb_callers[block.id] > 1:
                            if 'ADDR' not in self.__protos.get_proto(block.id)[1:]:
                                nb_new_addr[block.id][0] += 1
                                addr_seen.add(block.val)
                    nb_new_addr[block.id][1] += 1
                call_stack.pop()
            ii += 1
        print ii
        ll = sorted(nb_new_addr.items(), key=lambda a: -a[1][0])
        candidates = list()
        for f in ll:
            proto = self.__protos.get_proto(f[0])
            if 'ADDR' not in proto[1:]:
                candidates.append(f[0])
        if len(candidates) > 0:
            return candidates[0]
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

    def candidates_from_couples(self, alloc_id):
        if self.__free_candidates.get(alloc_id) is None:
        #Iter on  the coupleres file ton find function who are candidates
        #to be the liberator
            free_candidates = dict()
            with open(self.__couples_file, 'r') as f:
                line = f.readline().split(':')
                while line != ['']:
                    if line[1] == alloc_id:
                        free_candidates[line[4]] = True
                    line = f.readline().split(':')
            self.__free_candidates[alloc_id] = free_candidates
        return self.__free_candidates[alloc_id]

    def block_not_in_couple(self, alloc, block):
        """
            Check that the block doesn't correspond to a couple between its
            function and the guessed allocator
        """
        function_id = block.id.split(':')[1]
        alloc_id = alloc.split(':')[1]
        r = self.candidates_from_couples(alloc_id).get(function_id) is None
        return r and (function_id != alloc_id)

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
            if (self.__couples_file is not None
                    and self.block_not_in_couple(ALLOC, block)):
                continue

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
        return map(lambda a: a[0].split("|"), free)
        # free[0][0].split("|")
        return free[0], free[1]

    def compute_blocks(self, ALLOC, FREE, POS):
        mem = Memory(debug=False)
        for i, block in enumerate(self.__parser.get()):
            if (self.__couples_file is not None
                    and self.block_not_in_couple(ALLOC, block)):
                        continue
            if block.id == ALLOC and block.is_out():
                mem.alloc(block.val, 1)
            elif block.id == FREE and block.is_in():
                mem.free(block.val)
            #if i % 5000 == 0:
            #    print "[errors] ALLOC: {0} | FREE: {1}".format(mem.errors[0], mem.errors[1])
            #    print "[allocs] CURR: {0} | TOTAL: {1}".format(*mem.allocated)
            #    print "[nbcall] ALLOC: {0} | FREE: {1}".format(*mem.nb_calls)

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
        print "{},{},{},{},{},{}".format(ALLOC, FREE, mem.nb_calls[0], mem.nb_calls[1], *mem.errors)
        return

    def run(self, libraries=False, wrappers=True, ignore=None,
            cli_ignore=None):
        if libraries:
            nb_callers = self.__compute_callers()
        else:
            nb_callers = None
        ALLOC = self.__alloc(nb_callers, libraries)
        test_error_rate = False
        if test_error_rate:
            ignore = list()
            while ALLOC is not None:
                FREES = self.__free(ALLOC, libraries)
                for FREE, POS in FREES[:3]:
                    # self.log("checking consistancy of blocks for ({},{})...".format(ALLOC, FREE))
                    self.compute_blocks(ALLOC, FREE, POS)
                ignore.append(ALLOC)
                ALLOC = self.__alloc(nb_callers, libraries, ignore=ignore)

        else:
            if ALLOC is None:
                self.log("allocator not found")
                return None, None
            ALLOC_IMAGE, ALLOC_ADDR, ALLOC_NAME = ALLOC.split(":")
            ALLOC_ADDR = hex(int(ALLOC.split(":")[1]))
            self.log("allocator found - {0}:{1}:{2}".format(ALLOC_IMAGE, ALLOC_ADDR, ALLOC_NAME))
            FREES = self.__free(ALLOC, libraries)
            try:
                FREE_IMAGE, FREE_ADDR, FREE_NAME = FREES[0][0].split(":")
            except IndexError:
                if self.__couples_file is not None:
                    self.log("Liberator not found in couples ! Aborting.")
                    return
                else:
                    self.log("Liberator not found ! Aborting.")
                    return

            FREE_ADDR = hex(int(FREE_ADDR))
            self.log("liberator found - {0}:{1}:{2}".format(FREE_IMAGE, FREE_ADDR, FREE_NAME))

            with open("log/{}_memcomb_{}.log".format(self.__pgm, int(time())), "w") as f:
                f.write(ALLOC + '\n')
                f.write(FREES[0][0])

            self.log("checking consistancy of blocks...")
            self.compute_blocks(ALLOC, *FREES[0])

        # if wrappers:
        #     # Detecting suballocators
        #     SUBALLOC = self.__wrappers(ALLOC)
        return ALLOC, FREES[0][0]


# class CallStack(object):
# 
#     def __init__(self, pgm):
#         self.__stack = list()
#         self.__depth_in_lib = 0
#         self.__pgm = pgm
# 
#     def pop(self):
#         fid = self.__stack.pop(-1)
#         if fid[0].id.split(":")[0] != self.__pgm:
#             self.__depth_in_lib -= 1
#         if self.depth < 0:
#             raise Exception("Inconsistancy in call stack")
#         return fid 
# 
#     def push(self, block):
#         # Only push if fid and date are different from the top
#         # of the stack
#         if len(self.__stack) > 0 and self.__stack[-1][0].id == block.id and self.__stack[-1][0].date == block.date:
#             self.__stack[-1].append(block)
#             return
#         if block.id.split(":")[0] != self.__pgm:
#             self.__depth_in_lib += 1
#         self.__stack.append([block])
#         
#     def expect(self, fid, date=None):
#         if len(self.__stack) == 0 or self.__stack[-1][0].id != fid:
#             print "ERROR IN CALL STACK"
#             print "[{2}] Expecting {0}, top is {1}".format(fid, self.top()[0].id, date)
#             print self.__stack
#             raise Exception
# 
#     def top(self):
#         if self.is_empty():
#             return None
#         return self.__stack[-1]
# 
#     def top_id(self):
#         if self.is_empty():
#             return None
#         return self.__stack[-1][0].id
# 
#     def is_empty(self):
#         return len(self.__stack) == 0
# 
#     def items(self):
#         for blocks in reversed(self.__stack):
#             yield blocks
# 
#     @property
#     def depth(self):
#         return self.__depth_in_lib
# 
#     @property
#     def stack(self):
#         return map(lambda a: a[0].id, self.__stack)
# 
# 
# class AddrTable(object):
# 
#     TABLE_SIZE = 10000
# 
#     def __init__(self, dic=False):
#         self.__addr = list()
#         self.__dic = dic
#         for i in xrange(AddrTable.TABLE_SIZE):
#             if self.__dic:
#                 self.__addr.append(dict())
#             else:
#                 self.__addr.append(list())
#         self.__curr_key = None
#         self.__curr_addr = None
# 
#     def contains(self, addr):
#         key = addr % AddrTable.TABLE_SIZE
#         if self.__dic:
#             return addr in self.__addr[key].keys()
#         else:
#             return addr in self.__addr[key]
# 
#     def add(self, addr):
#         key = addr % AddrTable.TABLE_SIZE
#         if self.__dic:
#             self.__addr[key][addr] = list()
#         else:
#             self.__addr[key].append(addr)
# 
#     def remove(self, addr):
#         key = addr % AddrTable.TABLE_SIZE
#         self.__addr[key].remove(addr)
# 
#     def add_dic(self, addr, fid):
#         if not self.__dic:
#             raise Exception
#         key = addr % AddrTable.TABLE_SIZE
#         self.__addr[key][addr].append(fid)
# 
#     def items(self):
#         for key in self.__addr:
#             if self.__dic:
#                 for addr, call in key.items():
#                     yield addr, call
#             else:
#                 for addr in key:
#                     yield addr
# 
# 
# class Wrapper(object):
# 
#     def __init__(self, wid, ratio):
#         self.__id = wid
#         self.__ratio = ratio
#         self.__next = list()
# 
#     @property
#     def id(self):
#         return self.__id
# 
#     @property
#     def ratio(self):
#         return self.__ratio
# 
#     @property
#     def next(self):
#         return self.__next
# 
#     def add_child(self, wrap):
#         self.__next.append(wrap)
# 
#     def get(self, wid, d):
#         if d == 0:
#             if wid == self.id:
#                 return self
#             else:
#                 return None
#         for n in self.next:
#             nw = n.get(wid, d - 1)
#             if nw:
#                 return nw
#         return None
# 
#     def to_str(self, d):
#         s = ""
#         for i in xrange(d):
#             s += "**"
#         s += "| " + str(self.id) + "\n"
#         for n in self.next:
#             s += n.to_str(d + 1)
#         return s
# 
# 
# class Memory(object):
# 
#     HASH_SIZE = 10
#     KEY_MASK = 2**HASH_SIZE - 1
#     VALUE_MASK = (2**(64 - HASH_SIZE) - 1) << HASH_SIZE
# 
#     def __init__(self, debug=False):
#         self.__ALLOCATED = list()
#         self.__size = dict()
#         self.__err_alloc = 0
#         self.__err_free = 0
#         self.__nb_allocated = 0
#         self.__nb_allocated_tot = 0
#         self.__nb_alloc = 0
#         self.__nb_free = 0
#         self.DEBUG = debug
#         for i in xrange(Memory.HASH_SIZE):
#             self.__ALLOCATED.append(list())
# 
#     @property
#     def errors(self):
#         return self.__err_alloc, self.__err_free
# 
#     @property
#     def allocated(self):
#         return reduce(lambda a, b: a + len(b), self.__ALLOCATED, 0), self.__nb_allocated_tot
# 
#     @property
#     def nb_calls(self):
#         return self.__nb_alloc, self.__nb_free
# 
#     def alloc(self, addr, size):
#         self.__nb_alloc += 1
#         err = False
#         # Check that each cell is free
#         for i in xrange(size):
#             if addr + i == 19095744 and size < 100:
#                 print addr, size
#             if self.is_allocated(addr + i):
#                 if not err:
#                     self.__err_alloc += 1
#                     err = True
#                 if self.DEBUG:
#                     print "nballoc: {0}".format(self.__nb_alloc)
#                     print("ALLOC({0}, {1}): cell {2} already allocated".format(addr, size, addr + i))
#             else:
#                 self.__ALLOCATED[(addr + i) % Memory.HASH_SIZE].append(addr + i)
#         self.__nb_allocated_tot += size
#         self.__size[addr] = size
#     
#     def free(self, addr):
#         if addr == 0:
#             return
#         self.__nb_free += 1
#         block = None
#         size = None
#         # Get size
#         for p, s in self.__size.items():
#             if addr == p: # >= p and addr < p + s:
#                 block = p
#                 size = s
#                 if False and block == addr:
#                     self.__size.pop(block)
#                 self.__size.pop(block)
#                 break
#         if block is None:
#             self.__err_free += 1
#             if self.DEBUG:
#                 print("FREE({0}): block not allocated".format(addr))
