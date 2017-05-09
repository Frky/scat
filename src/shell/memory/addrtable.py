#-*- coding: utf-8 -*-

class AddrTable(object):

    TABLE_SIZE = 10000

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

    def remove(self, addr):
        key = addr % AddrTable.TABLE_SIZE
        self.__addr[key].remove(addr)

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
