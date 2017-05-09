# -*- coding: utf-8 -*-

class CallStack(object):

    def __init__(self, pgm):
        self.__stack = list()
        self.__depth_in_lib = 0
        self.__pgm = pgm

    def pop(self):
        fid = self.__stack.pop(-1)
        if fid[0].id.split(":")[0] != self.__pgm:
            self.__depth_in_lib -= 1
        if self.depth < 0:
            raise Exception("Inconsistancy in call stack")
        return fid 

    def push(self, block):
        # Only push if fid and date are different from the top
        # of the stack
        if len(self.__stack) > 0 and self.__stack[-1][0].id == block.id and self.__stack[-1][0].date == block.date:
            self.__stack[-1].append(block)
            return
        if block.id.split(":")[0] != self.__pgm:
            self.__depth_in_lib += 1
        self.__stack.append([block])
        
    def expect(self, fid, date=None):
        if len(self.__stack) == 0 or self.__stack[-1][0].id != fid:
            print "ERROR IN CALL STACK"
            print "[{2}] Expecting {0}, top is {1}".format(fid, self.top()[0].id, date)
            print self.__stack
            raise Exception

    def top(self):
        if self.is_empty():
            return None
        return self.__stack[-1]

    def top_id(self):
        if self.is_empty():
            return None
        return self.__stack[-1][0].id

    def is_empty(self):
        return len(self.__stack) == 0

    def items(self):
        for blocks in reversed(self.__stack):
            yield blocks

    @property
    def depth(self):
        return self.__depth_in_lib

    @property
    def stack(self):
        return map(lambda a: a[0].id, self.__stack)


