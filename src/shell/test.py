
from confiture import Confiture, ConfigFileError
from src.shell.data.data import Data


class ScatTest(object):
    
    def __init__(self, *args, **kwargs):
        self.__config = kwargs["clang"]
        self.__accuracy = {"arity": list(), "type": list()}

    def display(self):
        print "Average on ({}):".format(", ".join(self.__pgm.keys()))
        ok = sum(map(lambda a: a[0], self.__accuracy["arity"]))
        tot = sum(map(lambda a: a[1], self.__accuracy["arity"]))
        print "| Arity:         {0}/{1} - {2:.2f}%".format(
                    ok, 
                    tot, 
                    ok*100.0/tot
                )
        ok = sum(map(lambda a: a[0], self.__accuracy["type"]))
        tot = sum(map(lambda a: a[1], self.__accuracy["type"]))
        print "| Type:         {0}/{1} - {2:.2f}%".format(
                    ok, 
                    tot, 
                    ok*100.0/tot
                )

    def out(self, m):
        print m

    def test_all(self, p_arity, p_type, config):
        conf = Confiture("config/templates/test.yaml")
        self.__pgm = conf.check_and_get("test/config/" + config)
        for pgm, data in self.__pgm.items():
            # Step One: execute program with arguments
            cmd = "{}/{}".format(data["bin"], pgm)
            self.out("Launching {0} inference on {1}".format(p_arity, cmd))
            p_arity.launch(cmd, data["args"].split(" ") + [" > /dev/null"], verbose=False)
            self.out("Launching {0} inference on {1}".format(p_type, cmd))
            p_type.launch(cmd, data["args"].split(" ") + [" > /dev/null"], verbose=False)
            # Step Two: parse source
            # Create a parser object
            src_data = Data(self.__config["data-path"], pgm)
            if src_data.parse(cmd, self.__config["lib-path"], data["src"], force=False, verbose=False):
                src_data.dump()
            else:
                src_data.load(verbose=False)
            # Finally, compare source with infered results
            self.__accuracy["arity"].append(p_arity.get_analysis(pgm, src_data).accuracy(get=True, verbose=False))
            self.__accuracy["type"].append(p_type.get_analysis(pgm, src_data).accuracy(get=True, verbose=False))
        self.display()
