#-*- coding: utf-8 -*-

from src.shell.parser.i_log_parser import ILogParser

class TypeLogParser(ILogParser):
    """
        Parser for type log file

    """

    def __init__(self, *args, **kwargs):
        self.__fn = None
        super(TypeLogParser, self).__init__(*args, **kwargs)

    def get(self):
        if self.__fn is None:
            self.__fn = dict()
            with open(self.log_path, "r") as log:
                # Skip first line
                log.readline()
                # Read parameters
                for p in log.readline()[:-1].split(":"):
                    k, v = p.split("=")
                    self._params[k] = v
                for line in log.readlines():
                    l = line[:-1].split(":")
                    name = ":".join(l[:3])
                    proto = l[-1].replace(" ", "").split(",")
                    self.__fn[name] = proto
                    yield name, proto
        else:
            for name, proto in self.__fn.items():
                yield name, proto

    def get_proto(self, fname):
        if self.__fn is None:
            # parse log
            for f, g in self.get():
                pass
        if fname in self.__fn.keys():
            # Remove confidence rate before returning proto
            return [arg[:arg.index("(")] if arg.count("(") > 0 else arg for arg in self.__fn[fname]]
        else:
            # TODO create specific exception
            print "ERROR: {0} not found -- aborting".format(fname)
            raise Exception

