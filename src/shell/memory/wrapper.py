#-*- coding: utf-8 -*-

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

