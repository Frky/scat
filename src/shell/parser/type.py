#-*- coding: utf-8 -*-

from src.shell.parser.i_log_parser import ILogParser

class TypeLogParser(ILogParser):
    """
        Parser for type log file

    """

    def __init__(self, *args, **kwargs):
        self.__fn = None
        super(TypeLogParser, self).__init__(*args, **kwargs)

    def __parse(self):
        self.__fn = dict()
        with open(self.log_path, "r") as log:
            for line in log.readlines():
                name, proto = line[:-1].split(":")[2:]
                self.__fn[name] = proto.replace(" ", "").split(",")

    def get(self):
        if self.__fn is None:
            self.__parse()
        return self.__fn

    def get_proto(self, fname):
        if self.__fn is None:
            self.__parse()
        if fname in self.__fn.keys():
            # Remove confidence rate before returning proto
            return [arg[:arg.index("(")] if arg.count("(") > 0 else arg for arg in self.__fn[fname]]
        else:
            # TODO create specific exception
            print "ERROR: {0} not found -- aborting".format(fname)
            raise Exception

