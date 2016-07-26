
from abc import ABCMeta, abstractmethod 


class LogParser(object):
    """
        Parser for log files

        This is an abstract class that will be concretised by
        parsers for each type of log file (arity log, type log, etc.)

    """


    __metaclass__ = ABCMeta


    def __init__(self, log_path):
        # Path to log file
        self.log_path = log_path
        

    @abstractmethod
    def parse(self):
        raise NotImplemented


class TypeLogParser(LogParser):
    """
        Parser for type log file

    """

    def __init__(self, *args, **kwargs):
        self.__fn = dict()
        super(TypeLogParser, self).__init__(*args, **kwargs)


    def parse(self):
        with open(self.log_path, "r") as log:
            for line in log.readlines():
                name, proto = line[:-1].split(":")[2:]
                self.__fn[name] = proto.replace(" ", "").split(",")


    def get_all(self):
        return self.__fn


    def get_proto(self, fname):
        if fname in self.__fn.keys():
            # Remove confidence rate before returning proto
            return [arg[:arg.index("(")] if arg.count("(") > 0 else arg for arg in self.__fn[fname]]
        else:
            # TODO create specific exception
            print "ERROR: {0} not found -- aborting".format(fname)
            raise Exception
