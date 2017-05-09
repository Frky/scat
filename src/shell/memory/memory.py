#-*- coding: utf-8 -*-

class Memory(object):

    HASH_SIZE = 10
    KEY_MASK = 2**HASH_SIZE - 1
    VALUE_MASK = (2**(64 - HASH_SIZE) - 1) << HASH_SIZE

    def __init__(self, debug=False):
        self.__ALLOCATED = list()
        self.__size = dict()
        self.__err_alloc = 0
        self.__err_free = 0
        self.__nb_allocated = 0
        self.__nb_allocated_tot = 0
        self.__nb_alloc = 0
        self.__nb_free = 0
        self.DEBUG = debug
        for i in xrange(Memory.HASH_SIZE):
            self.__ALLOCATED.append(list())

    @property
    def errors(self):
        return self.__err_alloc, self.__err_free

    @property
    def allocated(self):
        return reduce(lambda a, b: a + len(b), self.__ALLOCATED, 0), self.__nb_allocated_tot

    @property
    def nb_calls(self):
        return self.__nb_alloc, self.__nb_free

    def alloc(self, addr, size):
        self.__nb_alloc += 1
        err = False
        # Check that each cell is free
        for i in xrange(size):
            if addr + i == 19095744 and size < 100:
                print addr, size
            if self.is_allocated(addr + i):
                if not err:
                    self.__err_alloc += 1
                    err = True
                if self.DEBUG:
                    print "nballoc: {0}".format(self.__nb_alloc)
                    print("ALLOC({0}, {1}): cell {2} already allocated".format(addr, size, addr + i))
            else:
                self.__ALLOCATED[(addr + i) % Memory.HASH_SIZE].append(addr + i)
        self.__nb_allocated_tot += size
        self.__size[addr] = size
    
    def free(self, addr):
        if addr == 0:
            return
        self.__nb_free += 1
        block = None
        size = None
        # Get size
        for p, s in self.__size.items():
            if addr == p: # >= p and addr < p + s:
                block = p
                size = s
                if False and block == addr:
                    self.__size.pop(block)
                self.__size.pop(block)
                break
        if block is None:
            self.__err_free += 1
            if self.DEBUG:
                print("FREE({0}): block not allocated".format(addr))
            return
        # Check that each cell is allocated
        for p in xrange(block, block + size):
            if not self.is_allocated(p):
                self.__err_free += 1
                if self.DEBUG:
                    print("FREE({0}, {1}): cell {2} not allocated (block beg: {3})".format(addr, size, p, block))
            else: 
                self.__ALLOCATED[(p) % Memory.HASH_SIZE].remove(p)

    def is_allocated(self, addr):
        return addr in self.__ALLOCATED[addr % Memory.HASH_SIZE]

